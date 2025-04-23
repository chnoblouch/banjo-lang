#include "inlining_pass.hpp"

#include "banjo/passes/pass_utils.hpp"
#include "banjo/passes/precomputing.hpp"
#include "banjo/ssa/basic_block.hpp"

#include <iostream>
#include <string>
#include <vector>

#define DEBUG_LOG is_logging() && log()

namespace banjo {

namespace passes {

constexpr int GAIN_BIAS = 3;

static int inlining_label = 0;

InliningPass::InliningPass(target::Target *target) : Pass("inlining", target) {
    // enable_logging(std::cout);
}

void InliningPass::run(ssa::Module &mod) {
    call_graph = ssa::CallGraph(mod);

    std::vector<ssa::Function *> roots;

    for (ssa::Function *func : mod.get_functions()) {
        if (func->global) {
            roots.push_back(func);
        }
    }

    for (ssa::Global *global : mod.get_globals()) {
        if (auto func = std::get_if<ssa::Function *>(&global->initial_value)) {
            roots.push_back(*func);
        }
    }

    for (ssa::Function *root : roots) {
        run(root);
    }

    DEBUG_LOG << "\n";
}

void InliningPass::run(ssa::Function *func) {
    if (funcs_visited.contains(func)) {
        return;
    }

    funcs_visited.insert(func);

    // First visit functions called in this function.
    // This has the effect that inlining at first happens deep in the call graph.
    for (ssa::BasicBlock &basic_block : func->get_basic_blocks()) {
        for (ssa::Instruction &instr : basic_block) {
            if (instr.get_opcode() == ssa::Opcode::CALL && instr.get_operand(0).is_func()) {
                run(instr.get_operand(0).get_func());
            }
        }
    }

    DEBUG_LOG << "\n  inliner: analyzing " << func->name << "\n";

    // Now try to inline call instructions in this function.
    for (ssa::BasicBlockIter block_iter = func->begin(); block_iter != func->end(); ++block_iter) {
        for (ssa::InstrIter instr_iter = block_iter->begin(); instr_iter != block_iter->end(); ++instr_iter) {
            if (instr_iter->get_opcode() == ssa::Opcode::CALL && instr_iter->get_operand(0).is_func()) {
                try_inline(func, block_iter, instr_iter);
            }
        }
    }

    Precomputing::precompute_instrs(*func);
}

void InliningPass::try_inline(ssa::Function *func, ssa::BasicBlockIter &block_iter, ssa::InstrIter &call_iter) {
    ssa::Instruction &call_instr = *call_iter;
    std::vector<ssa::Operand> call_operands = call_instr.get_operands();
    ssa::Function *callee = call_instr.get_operand(0).get_func();

    DEBUG_LOG << "    ";

    if (!is_inlining_beneficial(func, callee)) {
        DEBUG_LOG << "skipped: " << callee->name << "\n";
        return;
    }

    if (!is_inlining_legal(func, callee)) {
        DEBUG_LOG << "blocked: " << func->name << "\n";
        return;
    }

    inline_func(*func, block_iter, call_iter);
    DEBUG_LOG << "inlined: " << callee->name << "\n";
}

InliningPass::CalleeInfo InliningPass::collect_info(ssa::Function *callee) {
    CalleeInfo info{
        .multiple_blocks = callee->get_basic_blocks().get_size() > 1,
        .multiple_returns = false,
    };

    return info;
}

void InliningPass::inline_func(ssa::Function &func, ssa::BasicBlockIter &block_iter, ssa::InstrIter &call_iter) {
    ssa::InstrIter instr_after_call = call_iter.get_next();

    ssa::BasicBlock &block = *block_iter;
    ssa::Instruction &call_instr = *call_iter;

    std::optional<ssa::VirtualRegister> dst = call_instr.get_dest();
    std::vector<ssa::Operand> call_operands = call_instr.get_operands();
    ssa::Function &callee = *call_instr.get_operand(0).get_func();

    bool is_single_block = callee.get_basic_blocks().get_size() == 1;

    ssa::BasicBlockIter end_block;
    if (!is_single_block) {
        end_block = func.split_block_after(block_iter, call_iter);
    }

    Context ctx{
        .call_instr = call_iter,
        .is_single_block = is_single_block,
        .end_block = end_block,
    };

    std::optional<ssa::Value> return_val;

    for (ssa::BasicBlock &callee_block : callee) {
        for (ssa::InstrIter iter = callee_block.begin(); iter != callee_block.end(); ++iter) {
            ssa::Instruction &callee_instr = *iter;

            if (callee_instr.get_opcode() == ssa::Opcode::LOADARG) {
                ssa::Value value = call_operands[callee_instr.get_operand(1).get_int_immediate().to_u64() + 1];
                ctx.reg2val[*callee_instr.get_dest()] = value;
                ctx.removed_instrs.insert(iter);
                continue;
            }

            if (callee_instr.get_opcode() == ssa::Opcode::RET) {
                if (!callee_instr.get_operands().empty() && dst) {
                    return_val = callee_instr.get_operand(0);
                }

                // The return instruction will not be inlined if there is only one block because
                // there won't be a need for a jump at the end of the inlined function.
                if (is_single_block) {
                    ctx.removed_instrs.insert(iter);
                }

                continue;
            }

            std::optional<ssa::Value> precomputed_result = Precomputing::precompute_result(callee_instr);
            if (precomputed_result) {
                ctx.reg2val[*callee_instr.get_dest()] = *precomputed_result;
                ctx.removed_instrs.insert(iter);
                continue;
            }

            if (callee_instr.get_dest()) {
                ctx.reg2reg.insert({*callee_instr.get_dest(), next_vreg++});
            }
        }
    }

    if (!is_single_block) {
        for (ssa::BasicBlockIter iter = callee.begin(); iter != callee.end(); ++iter) {
            ssa::BasicBlock inline_block("inlined." + std::to_string(inlining_label++));

            if (iter != callee.get_entry_block_iter()) {
                inline_block.get_param_regs().resize(iter->get_param_regs().size());
                inline_block.get_param_types().resize(iter->get_param_regs().size());

                for (unsigned i = 0; i < iter->get_param_regs().size(); i++) {
                    ssa::VirtualRegister inline_param_reg = next_vreg++;
                    inline_block.get_param_regs()[i] = inline_param_reg;
                    inline_block.get_param_types()[i] = iter->get_param_types()[i];
                    ctx.reg2reg.insert({iter->get_param_regs()[i], inline_param_reg});
                }
            }

            ssa::BasicBlockIter inline_block_iter = func.get_basic_blocks().insert_before(ctx.end_block, inline_block);
            ctx.block_map.insert({iter, inline_block_iter});
        }
    } else {
        ctx.block_map.insert({callee.begin(), block_iter});
    }

    for (ssa::BasicBlockIter block_iter = callee.begin(); block_iter != callee.end(); ++block_iter) {
        ssa::BasicBlockIter inline_block = ctx.block_map[block_iter];

        for (ssa::InstrIter instr_iter = block_iter->begin(); instr_iter != block_iter->end(); ++instr_iter) {
            inline_instr(instr_iter, *inline_block, ctx);
        }
    }

    if (return_val) {
        return_val = get_inlined_value(*return_val, ctx);
    }

    if (!is_single_block) {
        ssa::BranchTarget entry_target{.block = block_iter.get_next(), .args = {}};
        block.append(ssa::Instruction(ssa::Opcode::JMP, {ssa::Operand::from_branch_target(entry_target)}));
    }

    if (return_val) {
        PassUtils::replace_in_func(func, *dst, *return_val);
    }

    block.remove(call_iter);

    if (is_single_block) {
        call_iter = instr_after_call.get_prev();
    } else {
        block_iter = end_block;
        call_iter = end_block->get_instrs().get_first_iter().get_prev();
    }
}

void InliningPass::inline_instr(ssa::InstrIter instr_iter, ssa::BasicBlock &block, Context &ctx) {
    if (ctx.removed_instrs.contains(instr_iter)) {
        return;
    }

    ssa::Instruction inline_instr = *instr_iter;

    if (inline_instr.get_opcode() == ssa::Opcode::RET) {
        ssa::BranchTarget target{.block = ctx.end_block, .args = {}};
        inline_instr = ssa::Instruction(ssa::Opcode::JMP, {ssa::Operand::from_branch_target(target)});
    }

    if (inline_instr.get_dest()) {
        auto dst_reg2reg_iter = ctx.reg2reg.find(*inline_instr.get_dest());
        if (dst_reg2reg_iter != ctx.reg2reg.end()) {
            inline_instr.set_dest(dst_reg2reg_iter->second);
        }
    }

    for (ssa::Operand &operand : inline_instr.get_operands()) {
        if (operand.is_register()) {
            operand = get_inlined_value(operand, ctx);
        } else if (operand.is_branch_target()) {
            ssa::BranchTarget &target = operand.get_branch_target();

            if (ctx.block_map.contains(target.block)) {
                target.block = ctx.block_map[target.block];
            }

            for (ssa::Operand &arg : target.args) {
                arg = get_inlined_value(arg, ctx);
            }
        }
    }

    if (ctx.is_single_block) {
        block.insert_before(ctx.call_instr, inline_instr);
    } else {
        block.append(inline_instr);
    }
}

ssa::Value InliningPass::get_inlined_value(ssa::Value &value, Context &ctx) {
    if (!value.is_register()) {
        return value;
    }

    auto reg2reg_iter = ctx.reg2reg.find(value.get_register());
    if (reg2reg_iter != ctx.reg2reg.end()) {
        return ssa::Value::from_register(reg2reg_iter->second, value.get_type());
    }

    auto reg2val_iter = ctx.reg2val.find(value.get_register());
    if (reg2val_iter != ctx.reg2val.end()) {
        return reg2val_iter->second.with_type(value.get_type());
    }

    return value;
}

int InliningPass::estimate_cost(ssa::Function &func) {
    /*
    int cost = 0;

    for(Instruction& instr : func.get_instructions()) {
        if(instr.get_opcode() == Opcode::RET) {
            continue;
        }

        if(instr.get_opcode() == Opcode::LOAD && instr.get_operand(1).is_register()) {
            bool is_loading_param = false;

            for(VirtualRegister param_reg : func.get_param_regs()) {
                if(param_reg == instr.get_operand(1).get_register()) {
                    is_loading_param = true;
                    break;
                }
            }

            if(is_loading_param) {
                continue;
            }
        }

        cost++;
    }

    return cost;
    */
    return 0;
}

int InliningPass::estimate_gain(ssa::Function &func) {
    return 2 * static_cast<int>(func.type.params.size()) + GAIN_BIAS;
}

bool InliningPass::is_inlining_beneficial(ssa::Function *caller, ssa::Function *callee) {
    if (call_graph.get_node(callee).preds.size() == 1) {
        return callee->get_basic_blocks().get_size() <= 64;
    }

    if (callee->get_basic_blocks().get_size() == 1) {
        return callee->get_entry_block_iter()->get_size() <= 64;
    }

    unsigned num_instrs = 0;
    for (ssa::BasicBlock &block : *callee) {
        num_instrs += block.get_instrs().get_size();
    }

    return num_instrs <= 24;
}

bool InliningPass::is_inlining_legal(ssa::Function *caller, ssa::Function *callee) {
    // Can't inline if this is a recursive function call.
    if (caller == callee) {
        return false;
    }

    // Can't inline if the caller has been inlined into the callee.
    // if (inlined_funcs.contains({callee, caller})) {
    //     return false;
    // }

    return true;
}

} // namespace passes

} // namespace banjo
