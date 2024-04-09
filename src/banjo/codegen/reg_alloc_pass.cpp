#include "reg_alloc_pass.hpp"

#include "emit/nasm_emitter.hpp"
#include "target/x86_64/x86_64_opcode.hpp"
#include "target/x86_64/x86_64_register.hpp"

#include "emit/debug_emitter.hpp"
#include "utils/timing.hpp"

#include <iostream>

namespace codegen {

RegAllocPass::RegAllocPass(target::TargetRegAnalyzer &analyzer) : analyzer(analyzer) {}

void RegAllocPass::run(mcode::Module &module_) {
    PROFILE_SCOPE("register allocation");

    // Some ideas on how to improve register allocation
    // - Give the register analyzer information about the current assigned registers
    //   so it can improve its suggestions.
    // - If two live ranges intersect, they may still be assigned the same physical register
    //   if the register is only read from during the intersection of the two ranges.

    for (mcode::Function *func : module_.get_functions()) {
        run(*func);
    }
}

void RegAllocPass::run(mcode::Function &func) {
    RegAllocFunc ra_func = create_reg_alloc_func(func);
    LivenessAnalysis liveness = LivenessAnalysis::compute(ra_func);

#if DEBUG_REG_ALLOC
    stream << "--- LIVENESS FOR " << func.get_name() << " ---" << std::endl;
    liveness.dump(stream);
    stream << std::endl;
#endif

    Context ctx{
        .func = ra_func,
        .liveness = liveness,
    };

    for (auto &iter : liveness.range_groups) {
        if (iter.first.is_physical_reg()) {
            reserve_range(ctx, iter.second, iter.first.get_physical_reg());
        }
    }

    for (KillPoint &kill_point : liveness.kill_points) {
        // Insert a range from the instruction's def to itself.
        RegAllocPoint ra_point{.instr = kill_point.instr, .stage = 1};
        RegAllocRange ra_range{(mcode::PhysicalReg)kill_point.reg, ra_point, ra_point};
        ra_func.blocks[kill_point.block].allocated_ranges.push_back(ra_range);
    }

    // for (auto &iter : liveness.range_groups) {
    //     iter.second.hints = analyzer.suggest_regs(ra_func, iter.second);
    // }

    Queue groups;
    for (auto &group : liveness.range_groups) {
        if (group.first.is_virtual_reg()) {
            groups.push(group.second);
        }
    }

    while (!groups.empty()) {
        LiveRangeGroup group = groups.top();
        groups.pop();

        Alloc alloc = alloc_group(ctx, group);
        ctx.reg_map.insert({group.reg.get_virtual_reg(), alloc});

        if (alloc.is_physical_reg) {
            reserve_range(ctx, group, alloc.physical_reg);
        }
    }

#if DEBUG_REG_ALLOC
    for (auto alloc : ctx.reg_map) {
        if (alloc.second.is_physical_reg) {
            std::string physical_reg_name = DebugEmitter::get_physical_reg_name(alloc.second.physical_reg, 8);
            stream << "%" << alloc.first << " -> " << physical_reg_name << std::endl;
        } else {
            stream << "%" << alloc.first << " -> spilled" << std::endl;
        }
    }

    stream << std::endl;
#endif

    for (auto alloc : ctx.reg_map) {
        replace_with_physical_reg(ctx, alloc.second);
    }

    for (mcode::BasicBlock &block : func.get_basic_blocks()) {
        remove_useless_instrs(block);
    }
}

RegAllocFunc RegAllocPass::create_reg_alloc_func(mcode::Function &func) {
    std::unordered_map<mcode::BasicBlockIter, unsigned> block_indices;

    unsigned index = 0;
    for (mcode::BasicBlockIter iter = func.begin(); iter != func.end(); ++iter) {
        block_indices.insert({iter, index++});
    }

    RegAllocFunc ra_func{func};

    for (mcode::BasicBlockIter iter = func.begin(); iter != func.end(); ++iter) {
        std::vector<unsigned> ra_preds;
        for (mcode::BasicBlockIter pred : iter->get_predecessors()) {
            ra_preds.push_back(block_indices[pred]);
        }

        std::vector<unsigned> ra_succs;
        for (mcode::BasicBlockIter succ : iter->get_successors()) {
            ra_succs.push_back(block_indices[succ]);
        }

        ra_func.blocks.push_back(RegAllocBlock{
            .m_block = iter,
            .instrs = collect_instrs(*iter),
            .preds = ra_preds,
            .succs = ra_succs,
        });
    }

    return ra_func;
}

std::vector<RegAllocInstr> RegAllocPass::collect_instrs(mcode::BasicBlock &basic_block) {
    std::vector<RegAllocInstr> instrs(basic_block.get_instrs().get_size());
    unsigned index = 0;

    for (mcode::InstrIter iter = basic_block.begin(); iter != basic_block.end(); ++iter) {
        std::vector<mcode::RegOp> regs = analyzer.get_operands(iter, basic_block);
        regs.insert(regs.end(), iter->get_reg_ops().begin(), iter->get_reg_ops().end());

        instrs[index] = RegAllocInstr{.index = index, .iter = iter, .regs = std::move(regs)};
        index += 1;
    }

    return instrs;
}

void RegAllocPass::reserve_range(Context &ctx, LiveRangeGroup &group, mcode::PhysicalReg reg) {
    for (LiveRange &range : group.ranges) {
        RegAllocRange ra_range = range.to_ra_range(reg);
        ctx.func.blocks[range.block].allocated_ranges.push_back(ra_range);
    }
}

RegAllocPass::Alloc RegAllocPass::alloc_group(Context &ctx, LiveRangeGroup &group) {
    for (mcode::PhysicalReg candidate : analyzer.suggest_regs(ctx.func, group)) {
        if (is_alloc_possible(ctx, group, candidate)) {
            return Alloc{
                .is_physical_reg = true,
                .physical_reg = (mcode::PhysicalReg)candidate,
                .group = group,
            };
        }
    }

    LiveRange &first_range = group.ranges[0];
    mcode::Instruction &first_instr = *ctx.func.blocks[first_range.block].instrs[first_range.start].iter;

    for (int candidate : analyzer.get_candidates(first_instr)) {
        if (is_alloc_possible(ctx, group, candidate)) {
            return Alloc{
                .is_physical_reg = true,
                .physical_reg = (mcode::PhysicalReg)candidate,
                .group = group,
            };
        }
    }

    return Alloc{
        .is_physical_reg = false,
        .stack_slot = ctx.func.m_func.get_stack_frame().new_stack_slot({mcode::StackSlot::Type::GENERIC, 8, 1}),
        .group = group,
    };
}

bool RegAllocPass::is_alloc_possible(Context &ctx, LiveRangeGroup &group, mcode::PhysicalReg reg) {
    for (LiveRange &range : group.ranges) {
        RegAllocRange ra_range = range.to_ra_range(reg);

        for (RegAllocRange &allocated_range : ctx.func.blocks[range.block].allocated_ranges) {
            if (allocated_range.intersects(ra_range)) {
                return false;
            }
        }
    }

    return true;
}

void RegAllocPass::replace_with_physical_reg(Context &ctx, Alloc &alloc) {
    for (LiveRange &range : alloc.group.ranges) {
        RegAllocBlock &block = ctx.func.blocks[range.block];
        ctx.block_iter = block.m_block;

        for (unsigned index = range.start; index <= range.end; index++) {
            mcode::VirtualReg vreg = alloc.group.reg.get_virtual_reg();

            if (alloc.is_physical_reg) {
                mcode::InstrIter iter = block.instrs[index].iter;

                for (mcode::Operand &operand : iter->get_operands()) {
                    try_replace(operand, vreg, alloc.physical_reg);
                }
            } else {
                insert_spilled_load_store(ctx, vreg, alloc, block.instrs[index]);
            }
        }
    }
}

void RegAllocPass::try_replace(
    mcode::Operand &operand,
    mcode::VirtualReg virtual_reg,
    mcode::PhysicalReg physical_reg
) {
    if (operand.is_virtual_reg()) {
        if ((mcode::VirtualReg)operand.get_virtual_reg() == virtual_reg) {
            operand.set_to_register(mcode::Register::from_physical(physical_reg));
        }
    } else if (operand.is_addr()) {
        mcode::IndirectAddress &addr = operand.get_addr();
        mcode::Register reg = mcode::Register::from_virtual(virtual_reg);

        if (addr.get_base() == reg) {
            addr.set_base(mcode::Register::from_physical(physical_reg));
        }

        if (addr.has_reg_offset() && addr.get_reg_offset() == reg) {
            addr.set_reg_offset(mcode::Register::from_physical(physical_reg));
        }
    } else if (operand.is_aarch64_addr()) {
        target::AArch64Address addr = operand.get_aarch64_addr();
        mcode::Register reg = mcode::Register::from_virtual(virtual_reg);

        if (addr.get_base() == reg) {
            addr.set_base(mcode::Register::from_physical(physical_reg));
        }

        if (addr.get_type() == target::AArch64Address::Type::BASE_OFFSET_REG && addr.get_offset_reg() == reg) {
            addr.set_offset_reg(mcode::Register::from_physical(physical_reg));
        }

        operand.set_to_aarch64_addr(addr);
    }
}

void RegAllocPass::insert_spilled_load_store(Context &ctx, mcode::VirtualReg vreg, Alloc &alloc, RegAllocInstr &instr) {
    for (mcode::RegOp &operand : instr.regs) {
        if (!operand.reg.is_virtual_reg(vreg)) {
            continue;
        }

        mcode::PhysicalReg tmp_reg = analyzer.insert_spill_reload({
            .instr_iter = instr.iter,
            .block = *ctx.block_iter,
            .stack_slot = alloc.stack_slot,
            .spill_tmp_regs = instr.spill_tmp_regs,
            .usage = operand.usage,
        });

        for (mcode::Operand &operand : instr.iter->get_operands()) {
            try_replace(operand, vreg, tmp_reg);
        }

        instr.spill_tmp_regs += 1;
    }
}

void RegAllocPass::remove_useless_instrs(mcode::BasicBlock &basic_block) {
    for (mcode::InstrIter iter = basic_block.begin(); iter != basic_block.end(); ++iter) {
        if (analyzer.is_instr_removable(*iter)) {
            mcode::InstrIter new_iter = iter.get_prev();
            basic_block.remove(iter);
            iter = new_iter;
        }
    }
}

bool RegAllocPass::GroupComparator::operator()(const LiveRangeGroup &lhs, const LiveRangeGroup &rhs) {
    return get_weight(lhs) < get_weight(rhs);
}

unsigned RegAllocPass::GroupComparator::get_weight(const LiveRangeGroup &group) {
    unsigned score = 0;

    // Longer ranges are allocated first as they tend to be more contrained.
    unsigned longest_range = 0;
    for (const LiveRange &range : group.ranges) {
        longest_range = std::max(longest_range, range.end - range.start);
    }
    score += longest_range;

    return score;
}

} // namespace codegen
