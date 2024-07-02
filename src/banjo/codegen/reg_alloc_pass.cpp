#include "reg_alloc_pass.hpp"

#include "codegen/liveness.hpp"
#include "codegen/reg_alloc_func.hpp"
#include "emit/debug_emitter.hpp"
#include "mcode/instruction.hpp"
#include "mcode/register.hpp"
#include "mcode/stack_frame.hpp"
#include "mcode/stack_slot.hpp"
#include "utils/timing.hpp"

#include <iostream>
#include <queue>
#include <vector>

namespace banjo {

namespace codegen {

RegAllocPass::RegAllocPass(target::TargetRegAnalyzer &analyzer) : analyzer(analyzer) {}

void RegAllocPass::run(mcode::Module &module_) {
    PROFILE_SCOPE("register allocation");

    // Things to be done:
    // - Bundle splitting
    // - Evicting bundles with a lower weight for bundles that were not produced by spilling
    // - If two live ranges intersect, the same physical register should be assigned
    //   if the register is only read from during the intersection of the two ranges
    // - Improving weight calculation (e.g. assign a higher score for bundles in loops)
    // - Smarter load/store placement for spilled bundles

    for (mcode::Function *func : module_.get_functions()) {
        run(*func);
    }
}

void RegAllocPass::run(mcode::Function &func) {
    RegAllocFunc ra_func = create_reg_alloc_func(func);
    LivenessAnalysis liveness = LivenessAnalysis::compute(ra_func);

    Context ctx{.func = ra_func, .liveness = liveness};

    assign_reg_classes(ctx);
    reserve_fixed_ranges(ctx);

    std::vector<Bundle> bundles = create_bundles(ctx);
    bundles = coalesce_bundles(ctx, bundles);

    BundleComparator comparator;
    ctx.bundles = Queue{comparator, bundles};

    while (!ctx.bundles.empty()) {
        Bundle bundle = ctx.bundles.top();
        ctx.bundles.pop();
        alloc_bundle(ctx, bundle);
    }

    write_debug_report(ctx);

    for (const Alloc &alloc : ctx.allocs) {
        apply_alloc(ctx, alloc);
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

void RegAllocPass::assign_reg_classes(Context &ctx) {
    for (mcode::BasicBlock &block : ctx.func.m_func) {
        for (mcode::Instruction &instr : block) {
            analyzer.assign_reg_classes(instr, ctx.reg_classes);
        }
    }
}

void RegAllocPass::reserve_fixed_ranges(Context &ctx) {
    // Reserve ranges for fixed physical registers.
    for (auto &[reg, ranges] : ctx.liveness.reg_ranges) {
        if (!reg.is_physical_reg()) {
            continue;
        }

        for (LiveRange &range : ranges) {
            ctx.func.blocks[range.block].reserved_segments.push_back({
                .reg = static_cast<mcode::PhysicalReg>(reg.get_physical_reg()),
                .range = range,
            });
        }
    }

    // Reserve ranges for kill points.
    for (KillPoint &kill_point : ctx.liveness.kill_points) {
        // Insert a range from the instruction's def to itself.

        RegAllocPoint ra_point{.instr = kill_point.instr, .stage = 1};

        ctx.func.blocks[kill_point.block].reserved_segments.push_back({
            .reg = kill_point.reg,
            .range{
                .block = kill_point.block,
                .start = ra_point,
                .end = ra_point,
            },
        });
    }
}

std::vector<Bundle> RegAllocPass::create_bundles(Context &ctx) {
    std::vector<Bundle> bundles;

    for (auto &[reg, ranges] : ctx.liveness.reg_ranges) {
        if (!reg.is_virtual_reg()) {
            continue;
        }

        Bundle bundle{
            .reg_class = ctx.reg_classes[reg.get_virtual_reg()],
        };

        for (const LiveRange &range : ranges) {
            bundle.segments.push_back({
                .reg = reg.get_virtual_reg(),
                .range = range,
            });
        }

        bundles.push_back(bundle);
    }

    return bundles;
}

std::vector<Bundle> RegAllocPass::coalesce_bundles(Context &ctx, std::vector<Bundle> bundles) {
    std::vector<Bundle> new_bundles;

    for (Bundle &bundle : bundles) {
        if (bundle.deleted) {
            continue;
        }

        for (Bundle &other : bundles) {
            if (!other.deleted) {
                try_coalesce(ctx, bundle, other);
            }
        }
    }

    for (Bundle &bundle : bundles) {
        if (!bundle.deleted) {
            new_bundles.push_back(bundle);
        }
    }

    return new_bundles;
}

void RegAllocPass::try_coalesce(Context &ctx, Bundle &a, Bundle &b) {
    if (a.reg_class != b.reg_class || b.segments.size() != 1) {
        return;
    }

    const Segment &segment_b = b.segments[0];
    const LiveRange &range_b = segment_b.range;

    for (const Segment &segment_a : a.segments) {
        const LiveRange &range_a = segment_a.range;

        if (range_a.block != range_b.block) {
            continue;
        }

        RegAllocPoint end = range_a.end;
        RegAllocPoint start = range_b.start;
        if (end.instr != start.instr || end.stage != 0 || start.stage != 1) {
            continue;
        }

        mcode::Instruction &instr = *ctx.func.blocks[range_a.block].instrs[end.instr].iter;
        if (!analyzer.is_move_from(instr, segment_a.reg) || intersect(a, b)) {
            continue;
        }

        a.segments.push_back(segment_b);
        b.deleted = true;
        return;
    }
}

bool RegAllocPass::intersect(const Bundle &a, const Bundle &b) {
    for (const Segment &segment_a : a.segments) {
        for (const Segment &segment_b : b.segments) {
            if (segment_a.range.intersects(segment_b.range)) {
                return true;
            }
        }
    }

    return false;
}

void RegAllocPass::alloc_bundle(Context &ctx, Bundle &bundle) {
    // First, try to allocate one of the registers suggested by the target.
    if (try_assign_suggested_regs(ctx, bundle)) {
        return;
    }

    // Next, try to allocate any register allowed for this instruction.
    if (try_assign_candidate_regs(ctx, bundle)) {
        return;
    }

    // Next, try to evict an existing allocation.
    if (try_evict(ctx, bundle)) {
        return;
    }

    // Finally, we're defeated and spill to the stack.
    spill(ctx, bundle);
}

bool RegAllocPass::try_assign_suggested_regs(Context &ctx, Bundle &bundle) {
    for (mcode::PhysicalReg candidate : analyzer.suggest_regs(ctx.func, bundle)) {
        if (try_alloc_physical_reg(ctx, bundle, candidate)) {
            return true;
        }
    }

    return false;
}

bool RegAllocPass::try_assign_candidate_regs(Context &ctx, Bundle &bundle) {
    for (mcode::PhysicalReg candidate : analyzer.get_candidates(bundle.reg_class)) {
        if (try_alloc_physical_reg(ctx, bundle, candidate)) {
            return true;
        }
    }

    return false;
}

bool RegAllocPass::try_alloc_physical_reg(Context &ctx, Bundle &bundle, mcode::PhysicalReg reg) {
    if (!is_alloc_possible(ctx, bundle, reg)) {
        return false;
    }

    Alloc alloc{.physical_reg = reg, .bundle = bundle};
    ctx.allocs.push_back(alloc);

    return true;
}

bool RegAllocPass::is_alloc_possible(Context &ctx, Bundle &bundle, mcode::PhysicalReg reg) {
    for (Segment &segment : bundle.segments) {
        for (ReservedSegment &reserved_segment : ctx.func.blocks[segment.range.block].reserved_segments) {
            if (reg == reserved_segment.reg && segment.range.intersects(reserved_segment.range)) {
                return false;
            }
        }

        for (const Alloc &alloc : ctx.allocs) {
            for (const Segment &alloc_range : alloc.bundle.segments) {
                if (alloc_range.range.block != segment.range.block) {
                    continue;
                }

                if (alloc.physical_reg == reg && alloc_range.range.intersects(segment.range)) {
                    return false;
                }
            }
        }
    }

    return true;
}

bool RegAllocPass::try_evict(Context &ctx, Bundle &bundle) {
    if (bundle.evictable) {
        return false;
    }

    for (auto iter = ctx.allocs.begin(); iter != ctx.allocs.end(); ++iter) {
        Alloc &alloc = *iter;

        if (!alloc.bundle.evictable || alloc.bundle.reg_class != bundle.reg_class) {
            continue;
        }

        ctx.bundles.push(alloc.bundle);
        alloc.bundle = bundle;
        return true;
    }

    return false;
}

void RegAllocPass::spill(Context &ctx, Bundle &bundle) {
    mcode::StackFrame &stack_frame = ctx.func.m_func.get_stack_frame();
    mcode::StackSlotID stack_slot = stack_frame.new_stack_slot({mcode::StackSlot::Type::GENERIC, 8, 1});

    for (Segment &range : bundle.segments) {
        const RegAllocBlock &block = ctx.func.blocks[range.range.block];

        for (unsigned instr = range.range.start.instr; instr <= range.range.end.instr; instr++) {
            for (mcode::RegOp op : block.instrs[instr].regs) {
                if (op.reg != mcode::Register::from_virtual(range.reg)) {
                    continue;
                }

                if (op.usage == mcode::RegUsage::DEF) {
                    LiveRange instr_range{.block = range.range.block, .start{instr, 1}, .end{instr, 1}};
                    Segment instr_segment{.reg = op.reg.get_virtual_reg(), .range = instr_range};

                    ctx.bundles.push({
                        .segments = {instr_segment},
                        .evictable = false,
                        .src_stack_slot = {},
                        .dst_stack_slot = stack_slot,
                    });
                } else if (op.usage == mcode::RegUsage::USE) {
                    LiveRange instr_range{.block = range.range.block, .start{instr, 0}, .end{instr, 0}};
                    Segment instr_segment{.reg = op.reg.get_virtual_reg(), .range = instr_range};

                    ctx.bundles.push({
                        .segments = {instr_segment},
                        .evictable = false,
                        .src_stack_slot = stack_slot,
                        .dst_stack_slot = {},
                    });
                } else if (op.usage == mcode::RegUsage::USE_DEF) {
                    LiveRange instr_range{.block = range.range.block, .start{instr, 0}, .end{instr, 1}};
                    Segment instr_segment{.reg = op.reg.get_virtual_reg(), .range = instr_range};

                    ctx.bundles.push({
                        .segments = {instr_segment},
                        .evictable = false,
                        .src_stack_slot = stack_slot,
                        .dst_stack_slot = stack_slot,
                    });
                }
            }
        }
    }
}

void RegAllocPass::apply_alloc(Context &ctx, const Alloc &alloc) {
    if (alloc.bundle.src_stack_slot) {
        const Segment &segment = alloc.bundle.segments[0];
        RegAllocBlock &block = ctx.func.blocks[segment.range.block];

        analyzer.insert_load({
            .instr_iter = block.instrs[segment.range.start.instr].iter,
            .block = *block.m_block,
            .stack_slot = *alloc.bundle.src_stack_slot,
            .reg = alloc.physical_reg,
        });
    }

    for (const Segment &segment : alloc.bundle.segments) {
        RegAllocBlock &block = ctx.func.blocks[segment.range.block];
        ctx.block_iter = block.m_block;

        for (unsigned index = segment.range.start.instr; index <= segment.range.end.instr; index++) {
            mcode::InstrIter instr = block.instrs[index].iter;

            for (mcode::Operand &operand : instr->get_operands()) {
                try_replace(operand, segment.reg, alloc.physical_reg);
            }
        }
    }

    if (alloc.bundle.dst_stack_slot) {
        const Segment &segment = alloc.bundle.segments[0];
        RegAllocBlock &block = ctx.func.blocks[segment.range.block];

        analyzer.insert_store({
            .instr_iter = block.instrs[segment.range.end.instr].iter,
            .block = *block.m_block,
            .stack_slot = *alloc.bundle.dst_stack_slot,
            .reg = alloc.physical_reg,
        });
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

void RegAllocPass::remove_useless_instrs(mcode::BasicBlock &basic_block) {
    for (mcode::InstrIter iter = basic_block.begin(); iter != basic_block.end(); ++iter) {
        if (analyzer.is_instr_removable(*iter)) {
            mcode::InstrIter new_iter = iter.get_prev();
            basic_block.remove(iter);
            iter = new_iter;
        }
    }
}

bool RegAllocPass::BundleComparator::operator()(const Bundle &lhs, const Bundle &rhs) {
    return get_weight(lhs) < get_weight(rhs);
}

unsigned RegAllocPass::BundleComparator::get_weight(const Bundle &bundle) {
    unsigned score = 0;

    // Longer ranges are allocated first as they tend to be more contrained.
    unsigned longest_range = 0;
    for (const Segment &segment : bundle.segments) {
        const LiveRange &range = segment.range;
        longest_range = std::max(longest_range, range.end.instr - range.start.instr);
    }
    score += longest_range;

    return score;
}

void RegAllocPass::write_debug_report(Context &ctx) {
#if DEBUG_REG_ALLOC
    stream << "--- LIVENESS FOR " << ctx.func.m_func.get_name() << " ---" << std::endl;
    ctx.liveness.dump(stream);
    stream << std::endl;

    for (const Alloc &alloc : ctx.allocs) {
        for (const Segment &segment : alloc.bundle.segments) {
            std::string physical_reg_name = DebugEmitter::get_physical_reg_name(alloc.physical_reg, 8);
            stream << "%" << segment.reg << " -> " << physical_reg_name << " in ";
            stream << segment.range.start.instr << ":" << segment.range.end.instr << "\n";
        }
    }

    stream << std::endl;
#endif
}

} // namespace codegen

} // namespace banjo
