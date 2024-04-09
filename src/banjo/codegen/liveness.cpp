#include "liveness.hpp"

#include "emit/debug_emitter.hpp"
#include "target/x86_64/x86_64_opcode.hpp"

#include <iostream>

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
                if (use.is_virtual_reg()) {
                    liveness.ins.insert(use);
                }
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

        for (mcode::Register out : analysis.block_liveness[block_index].outs) {
            analysis.range_groups[out].ranges.push_back({block_index, 0, (unsigned)block.instrs.size() - 1});
            live_regs.insert(out);
        }

        for (int instr_index = block.instrs.size() - 1; instr_index >= 0; instr_index--) {
            RegAllocInstr &instr = block.instrs[instr_index];

            for (mcode::RegOp reg : instr.regs) {
                if (reg.usage == mcode::RegUsage::USE) {
                    if (!live_regs.contains(reg.reg)) {
                        std::vector<LiveRange> &ranges = analysis.range_groups[reg.reg].ranges;
                        ranges.push_back({block_index, (unsigned)instr_index, (unsigned)instr_index});
                        live_regs.insert(reg.reg);
                    }
                } else if (reg.usage == mcode::RegUsage::DEF) {
                    if (live_regs.contains(reg.reg)) {
                        std::vector<LiveRange> &ranges = analysis.range_groups[reg.reg].ranges;
                        ranges.back().start = instr_index;
                        live_regs.erase(reg.reg);
                    } else {
                        // Insert a range with the length of one instruction if there is no use for this def.
                        std::vector<LiveRange> &ranges = analysis.range_groups[reg.reg].ranges;
                        ranges.push_back({block_index, (unsigned)instr_index, (unsigned)instr_index});
                    }
                } else if (reg.usage == mcode::RegUsage::USE_DEF) {
                    // Insert a range with the length of one instruction if there is no use for this use-def.
                    if (!live_regs.contains(reg.reg)) {
                        std::vector<LiveRange> &ranges = analysis.range_groups[reg.reg].ranges;
                        ranges.push_back({block_index, (unsigned)instr_index, (unsigned)instr_index});
                        live_regs.insert(reg.reg);
                    }
                } else if (reg.usage == mcode::RegUsage::KILL) {
                    analysis.kill_points.push_back({
                        .reg = reg.reg.get_physical_reg(),
                        .block = block_index,
                        .instr = (unsigned)instr_index,
                    });

                    live_regs.erase(reg.reg);
                }
            }
        }

        for (mcode::Register in : analysis.block_liveness[block_index].ins) {
            analysis.range_groups[in].ranges.back().start = 0;
        }
    }

    for (auto &group : analysis.range_groups) {
        group.second.reg = group.first;
    }
}

void LivenessAnalysis::dump(std::ostream &stream) {
    stream << "useless:";

    for (const auto &group : range_groups) {
        if (!group.first.is_virtual_reg()) continue;

        bool used = false;

        for (const LiveRange &range : group.second.ranges) {
            for (mcode::RegOp op : func.blocks[range.block].instrs[range.end].regs) {
                if (op.reg == group.first &&
                    (op.usage == mcode::RegUsage::USE || op.usage == mcode::RegUsage::USE_DEF)) {
                    used = true;
                }
            }
        }

        if (!used) stream << " %" << group.first.get_virtual_reg();
    }

    stream << std::endl;

    unsigned long long interval_spacing = 50;
    std::string header = "vregs: ";
    header += std::string(std::min(interval_spacing - header.size(), interval_spacing), ' ');

    std::vector<unsigned> line_widths;

    for (const auto &group : range_groups) {
        std::string vreg_header;

        if (group.first.is_virtual_reg()) {
            vreg_header = '%' + std::to_string(group.first.get_virtual_reg());
        } else {
            vreg_header = DebugEmitter::get_physical_reg_name(group.first.get_physical_reg(), 8);
        }

        header += vreg_header + ' ';
        line_widths.push_back(vreg_header.size());
    }

    stream << header << std::endl;

    for (unsigned i = 0; i < func.blocks.size(); i++) {
        RegAllocBlock &block = func.blocks[i];

        const std::string &name = block.m_block->get_label();
        stream << (name.empty() ? "<entry>" : name) << std::endl;

        std::vector<std::vector<char>> lines(block.instrs.size(), std::vector<char>(range_groups.size(), '.'));
        unsigned vreg_index = 0;

        if (lines.empty()) {
            continue;
        }

        for (const auto &group : range_groups) {
            for (LiveRange live_range : group.second.ranges) {
                if (live_range.block != i) {
                    continue;
                }

                for (unsigned j = live_range.start; j <= live_range.end; j++) {
                    char c;
                    if (live_range.start == live_range.end) {
                        c = '#';
                    } else {
                        c = (j == live_range.start || j == live_range.end) ? 'o' : '|';
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
