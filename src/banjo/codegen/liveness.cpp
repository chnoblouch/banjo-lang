#include "liveness.hpp"

#include "banjo/codegen/reg_alloc_func.hpp"
#include "banjo/emit/debug_emitter.hpp"
#include "banjo/mcode/register.hpp"

#include <iostream>

namespace banjo {

namespace codegen {

LivenessAnalysis::LivenessAnalysis(RegAllocFunc &func) : func(func) {}

LivenessAnalysis LivenessAnalysis::compute(RegAllocFunc &func) {
    // FIXME: movs with the same source and destination break the liveness analysis.

    LivenessAnalysis result(func);
    result.block_liveness.resize(func.blocks.size());

    for (unsigned i = 0; i < func.blocks.size(); i++) {
        collect_uses_and_defs(func.blocks[i], result.block_liveness[i]);
    }

    compute_ins_and_outs(func, result);
    compute_precise_live_ranges(func, result);

    return result;
}

void LivenessAnalysis::collect_uses_and_defs(RegAllocBlock &block, BlockLiveness &liveness) {
    for (RegAllocInstr &instr : block.instrs) {
        for (mcode::RegOp &operand : instr.regs) {
            if (operand.usage == mcode::RegUsage::DEF) {
                liveness.defs.insert(operand.reg);
            } else if (operand.usage == mcode::RegUsage::USE && !liveness.defs.contains(operand.reg)) {
                liveness.uses.insert(operand.reg);
            }
        }
    }
}

void LivenessAnalysis::compute_ins_and_outs(RegAllocFunc &func, LivenessAnalysis &analysis) {
    bool changes = true;

    std::vector<unsigned> block_indices_post_order;
    std::unordered_set<unsigned> blocks_visited;
    collect_blocks_post_order(func, 0, block_indices_post_order, blocks_visited);

    while (changes) {
        changes = false;

        for (int i = block_indices_post_order.size() - 1; i >= 0; i--) {
            unsigned block_index = block_indices_post_order[i];
            RegAllocBlock &block = func.blocks[block_index];
            BlockLiveness &liveness = analysis.block_liveness[block_index];

            unsigned prev_num_ins = liveness.ins.size();
            unsigned prev_num_outs = liveness.outs.size();

            // Insert all ins from successors into the outs of the current block.
            for (unsigned succ : block.succs) {
                BlockLiveness &succ_liveness = analysis.block_liveness[succ];
                liveness.outs.insert(succ_liveness.ins.begin(), succ_liveness.ins.end());
            }

            // Insert the uses into the ins.
            for (mcode::Register use : liveness.uses) {
                liveness.ins.insert(use);
            }

            // Insert the outs that are not redefined into the ins.
            for (mcode::Register out : liveness.outs) {
                if (!liveness.defs.contains(out)) {
                    liveness.ins.insert(out);
                }
            }

            // Run another iteration if there were changes.
            changes = changes || (liveness.ins.size() != prev_num_ins || liveness.outs.size() != prev_num_outs);
        }
    }
}

void LivenessAnalysis::collect_blocks_post_order(
    RegAllocFunc &func,
    unsigned block_index,
    std::vector<unsigned> &indices,
    std::unordered_set<unsigned> &blocks_visited
) {
    blocks_visited.insert({block_index, 0});

    RegAllocBlock &block = func.blocks[block_index];

    for (unsigned succ : block.succs) {
        if (!blocks_visited.contains(succ)) {
            collect_blocks_post_order(func, succ, indices, blocks_visited);
        }
    }

    indices.push_back(block_index);
}

void LivenessAnalysis::compute_precise_live_ranges(RegAllocFunc &func, LivenessAnalysis &analysis) {
    for (unsigned block_index = 0; block_index < func.blocks.size(); block_index++) {
        RegAllocBlock &block = func.blocks[block_index];

        if (block.instrs.empty()) {
            continue;
        }

        std::unordered_set<mcode::Register> live_regs;

        const BlockLiveness &block_liveness = analysis.block_liveness[block_index];

        for (mcode::Register out : block_liveness.outs) {
            RegAllocPoint start{0, 0};
            RegAllocPoint end{static_cast<unsigned>(block.instrs.size() - 1), 1};
            analysis.reg_ranges[out].push_back({block_index, start, end});
            live_regs.insert(out);
        }

        for (int i = block.instrs.size() - 1; i >= 0; i--) {
            RegAllocInstr &instr = block.instrs[i];
            unsigned instr_index = static_cast<unsigned>(i);

            for (mcode::RegOp reg : instr.regs) {
                if (reg.usage == mcode::RegUsage::USE) {
                    if (!live_regs.contains(reg.reg)) {
                        std::vector<LiveRange> &ranges = analysis.reg_ranges[reg.reg];
                        ranges.push_back({block_index, {instr_index, 0}, {instr_index, 0}});
                        live_regs.insert(reg.reg);
                    }
                } else if (reg.usage == mcode::RegUsage::DEF) {
                    if (live_regs.contains(reg.reg)) {
                        std::vector<LiveRange> &ranges = analysis.reg_ranges[reg.reg];
                        ranges.back().start = {instr_index, 1};
                        live_regs.erase(reg.reg);
                    } else {
                        // Insert a range with the length of one instruction if there is no use for this def.
                        std::vector<LiveRange> &ranges = analysis.reg_ranges[reg.reg];
                        ranges.push_back({block_index, {instr_index, 1}, {instr_index, 1}});
                    }
                } else if (reg.usage == mcode::RegUsage::USE_DEF) {
                    // Insert a range with the length of one instruction if there is no use for this use-def.
                    if (!live_regs.contains(reg.reg)) {
                        std::vector<LiveRange> &ranges = analysis.reg_ranges[reg.reg];
                        ranges.push_back({block_index, {instr_index, 0}, {instr_index, 1}});
                        live_regs.insert(reg.reg);
                    }
                } else if (reg.usage == mcode::RegUsage::KILL) {
                    analysis.kill_points.push_back({
                        .reg = static_cast<mcode::PhysicalReg>(reg.reg.get_physical_reg()),
                        .block = block_index,
                        .instr = (unsigned)instr_index,
                    });

                    live_regs.erase(reg.reg);
                }
            }
        }

        for (mcode::Register in : block_liveness.ins) {
            analysis.reg_ranges[in].back().start = {0, 0};
        }
    }
}

void LivenessAnalysis::dump(std::ostream &stream) {
    stream << "useless:";

    for (const auto &[reg, ranges] : reg_ranges) {
        if (!reg.is_virtual_reg()) continue;

        bool used = false;

        for (const LiveRange &range : ranges) {
            for (mcode::RegOp op : func.blocks[range.block].instrs[range.end.instr].regs) {
                if (op.reg == reg && (op.usage == mcode::RegUsage::USE || op.usage == mcode::RegUsage::USE_DEF)) {
                    used = true;
                }
            }
        }

        if (!used) stream << " %" << reg.get_virtual_reg();
    }

    stream << std::endl;

    unsigned long long interval_spacing = 50;
    std::string header = "vregs: ";
    header += std::string(std::min(interval_spacing - header.size(), interval_spacing), ' ');

    std::vector<unsigned> line_widths;

    for (const auto &[reg, ranges] : reg_ranges) {
        std::string vreg_header;

        if (reg.is_virtual_reg()) {
            vreg_header = '%' + std::to_string(reg.get_virtual_reg());
        } else {
            vreg_header = DebugEmitter::get_physical_reg_name(reg.get_physical_reg(), 8);
        }

        header += vreg_header + ' ';
        line_widths.push_back(vreg_header.size());
    }

    stream << header << std::endl;

    for (unsigned i = 0; i < func.blocks.size(); i++) {
        RegAllocBlock &block = func.blocks[i];

        const std::string &name = block.m_block->get_label();
        stream << (name.empty() ? "<entry>" : name) << std::endl;

        std::vector<std::vector<char>> lines(block.instrs.size(), std::vector<char>(reg_ranges.size(), '.'));
        unsigned vreg_index = 0;

        if (lines.empty()) {
            continue;
        }

        for (const auto &[reg, ranges] : reg_ranges) {
            for (LiveRange live_range : ranges) {
                if (live_range.block != i) {
                    continue;
                }

                for (unsigned j = live_range.start.instr; j <= live_range.end.instr; j++) {
                    char c;
                    if (live_range.start.instr == live_range.end.instr) {
                        c = '#';
                    } else if (j == live_range.start.instr) {
                        c = "ud"[live_range.start.stage];
                    } else if (j == live_range.end.instr) {
                        c = "ud"[live_range.end.stage];
                    } else {
                        c = '|';
                    }

                    lines[j][vreg_index] = c;
                }
            }

            vreg_index += 1;
        }

        for (unsigned j = 0; j < block.instrs.size(); j++) {
            std::string instr = DebugEmitter::instr_to_string(*block.m_block, *block.instrs[j].iter);
            instr = instr.substr(0, interval_spacing);

            std::string spaces = std::string(std::min(interval_spacing - instr.size(), interval_spacing), ' ');
            stream << instr << spaces;

            for (unsigned k = 0; k < lines[j].size(); k++) {
                unsigned width = line_widths[k];
                unsigned half_width = line_widths[k] / 2;
                std::string pre_spaces(half_width, ' ');
                std::string post_spaces(width - half_width, ' ');
                stream << pre_spaces << lines[j][k] << post_spaces;
            }

            std::string killed_str;
            for (KillPoint &kill_point : kill_points) {
                if (kill_point.block == i && kill_point.instr == j) {
                    killed_str += DebugEmitter::get_physical_reg_name(kill_point.reg, 8) + " ";
                }
            }

            if (!killed_str.empty()) {
                stream << "killed: " << killed_str;
            }

            stream << std::endl;
        }
    }
}

} // namespace codegen

} // namespace banjo
