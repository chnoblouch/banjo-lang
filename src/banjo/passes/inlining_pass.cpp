#include "inlining_pass.hpp"

#include "passes/pass_utils.hpp"
#include "passes/precomputing.hpp"

#include <iostream>
#include <string>
#include <vector>

namespace banjo {

namespace passes {

constexpr int GAIN_BIAS = 3;

static int inlining_label = 0;

InliningPass::InliningPass(target::Target *target) : Pass("inlining", target) {
    enable_logging(std::cout);
}

void InliningPass::run(ir::Module &mod) {
    call_graph = ir::CallGraph(mod);

    for (ir::Function *func : mod.get_functions()) {
        if (func->is_global()) {
            run(func);
        }
    }
}

void InliningPass::run(ir::Function *func) {
    if (funcs_visited.contains(func)) {
        return;
    }

    funcs_visited.insert(func);

    // First visit functions called in this function.
    // This has the effect that inlining at first happens deep in the call graph.
    for (ir::BasicBlock &basic_block : func->get_basic_blocks()) {
        for (ir::Instruction &instr : basic_block) {
            if (instr.get_opcode() == ir::Opcode::CALL && instr.get_operand(0).is_func()) {
                run(instr.get_operand(0).get_func());
            }
        }
    }

    // Now try to inline call instructions in this function.
    for (ir::BasicBlockIter block_iter = func->begin(); block_iter != func->end(); ++block_iter) {
        for (ir::InstrIter instr_iter = block_iter->begin(); instr_iter != block_iter->end(); ++instr_iter) {
            if (instr_iter->get_opcode() == ir::Opcode::CALL && instr_iter->get_operand(0).is_func()) {
                try_inline(func, block_iter, instr_iter);
            }
        }
    }
}

void InliningPass::try_inline(ir::Function *func, ir::BasicBlockIter &block_iter, ir::InstrIter &call_iter) {
    ir::Instruction &call_instr = *call_iter;
    std::optional<ir::VirtualRegister> dest = call_instr.get_dest();
    std::vector<ir::Operand> call_operands = call_instr.get_operands();
    ir::Function *callee = call_instr.get_operand(0).get_func();

    ir::InstrIter instr_after_call = call_iter.get_next();

    if (!is_inlining_beneficial(func, callee)) {
        if (is_logging()) {
            get_logging_stream() << "not inlining " << callee->get_name() << " into " << func->get_name() << std::endl;
        }

        return;
    }

    if (!is_inlining_legal(func, callee)) {
        if (is_logging()) {
            get_logging_stream() << callee->get_name() << " -> " << func->get_name() << " is illegal\n";
        }

        return;
    }

    if (is_logging()) {
        get_logging_stream() << "inlining " << callee->get_name() << " into " << func->get_name() << std::endl;
    }

    bool single_block = callee->get_basic_blocks().get_size() == 1;

    ir::BasicBlockIter end_block;
    if (!single_block) {
        end_block = func->split_block_after(block_iter, call_iter);
    }

    Context ctx{
        .call_instr = call_iter,
        .single_block = single_block,
        .end_block = end_block,
    };

    std::optional<ir::Value> return_val;

    for (ir::BasicBlock &callee_block : *callee) {
        for (ir::InstrIter iter = callee_block.begin(); iter != callee_block.end(); ++iter) {
            ir::Instruction &callee_instr = *iter;

            if (callee_instr.get_opcode() == ir::Opcode::LOADARG) {
                ir::Value value = call_operands[callee_instr.get_operand(1).get_int_immediate().to_u64() + 1];
                ctx.reg2val[*callee_instr.get_dest()] = value;
                ctx.removed_instrs.insert(iter);
                continue;
            }

            if (callee_instr.get_opcode() == ir::Opcode::RET) {
                if (!callee_instr.get_operands().empty() && dest) {
                    return_val = callee_instr.get_operand(0);
                }

                // The return instruction will not be inlined if there is only one block because
                // there won't be a need for a jump at the end of the inlined function.
                if (single_block) {
                    ctx.removed_instrs.insert(iter);
                }

                continue;
            }

            std::optional<ir::Value> precomputed_result = Precomputing::precompute_result(callee_instr);
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

    if (!single_block) {
        for (ir::BasicBlockIter iter = callee->begin(); iter != callee->end(); ++iter) {
            ir::BasicBlock inline_block("inlined." + std::to_string(inlining_label++));

            if (iter != callee->get_entry_block_iter()) {
                inline_block.get_param_regs().resize(iter->get_param_regs().size());
                inline_block.get_param_types().resize(iter->get_param_regs().size());

                for (unsigned i = 0; i < iter->get_param_regs().size(); i++) {
                    ir::VirtualRegister inline_param_reg = next_vreg++;
                    inline_block.get_param_regs()[i] = inline_param_reg;
                    inline_block.get_param_types()[i] = iter->get_param_types()[i];
                    ctx.reg2reg.insert({iter->get_param_regs()[i], inline_param_reg});
                }
            }

            ir::BasicBlockIter inline_block_iter = func->get_basic_blocks().insert_before(end_block, inline_block);
            ctx.block_map.insert({iter, inline_block_iter});
        }
    } else {
        ctx.block_map.insert({callee->begin(), block_iter});
    }

    for (ir::BasicBlockIter block_iter = callee->begin(); block_iter != callee->end(); ++block_iter) {
        ir::BasicBlockIter inline_block = ctx.block_map[block_iter];

        for (ir::InstrIter instr_iter = block_iter->begin(); instr_iter != block_iter->end(); ++instr_iter) {
            inline_instr(instr_iter, *inline_block, ctx);
        }
    }

    if (return_val) {
        return_val = get_inlined_value(*return_val, ctx);
    }

    if (!single_block) {
        ir::BranchTarget entry_target{.block = block_iter.get_next(), .args = {}};
        block_iter->append(ir::Instruction(ir::Opcode::JMP, {ir::Operand::from_branch_target(entry_target)}));
    }

    if (return_val) {
        PassUtils::replace_in_func(*func, *call_instr.get_dest(), *return_val);
    }

    block_iter->remove(call_iter);

    if (single_block) {
        call_iter = instr_after_call.get_prev();
    } else {
        block_iter = end_block;
        call_iter = end_block->get_instrs().get_first_iter().get_prev();
    }

    Precomputing::precompute_instrs(*func);
}

InliningPass::CalleeInfo InliningPass::collect_info(ir::Function *callee) {
    CalleeInfo info{
        .multiple_blocks = callee->get_basic_blocks().get_size() > 1,
        .multiple_returns = false,
    };

    return info;
}

void InliningPass::inline_instr(ir::InstrIter instr_iter, ir::BasicBlock &block, Context &ctx) {
    if (ctx.removed_instrs.contains(instr_iter)) {
        return;
    }

    ir::Instruction inline_instr = *instr_iter;

    if (inline_instr.get_opcode() == ir::Opcode::RET) {
        ir::BranchTarget target{.block = ctx.end_block, .args = {}};
        inline_instr = ir::Instruction(ir::Opcode::JMP, {ir::Operand::from_branch_target(target)});
    }

    if (inline_instr.get_dest()) {
        auto dst_reg2reg_iter = ctx.reg2reg.find(*inline_instr.get_dest());
        if (dst_reg2reg_iter != ctx.reg2reg.end()) {
            inline_instr.set_dest(dst_reg2reg_iter->second);
        }
    }

    for (ir::Operand &operand : inline_instr.get_operands()) {
        if (operand.is_register()) {
            operand = get_inlined_value(operand, ctx);
        } else if (operand.is_branch_target()) {
            ir::BranchTarget &target = operand.get_branch_target();

            if (ctx.block_map.contains(target.block)) {
                target.block = ctx.block_map[target.block];
            }

            for (ir::Operand &arg : target.args) {
                arg = get_inlined_value(arg, ctx);
            }
        }
    }

    if (ctx.single_block) {
        block.insert_before(ctx.call_instr, inline_instr);
    } else {
        block.append(inline_instr);
    }
}

ir::Value InliningPass::get_inlined_value(ir::Value &value, Context &ctx) {
    if (!value.is_register()) {
        return value;
    }

    auto reg2reg_iter = ctx.reg2reg.find(value.get_register());
    if (reg2reg_iter != ctx.reg2reg.end()) {
        return ir::Value::from_register(reg2reg_iter->second, value.get_type());
    }

    auto reg2val_iter = ctx.reg2val.find(value.get_register());
    if (reg2val_iter != ctx.reg2val.end()) {
        return reg2val_iter->second.with_type(value.get_type());
    }

    return value;
}

int InliningPass::estimate_cost(ir::Function &func) {
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

int InliningPass::estimate_gain(ir::Function &func) {
    return 2 * (int)(func.get_params().size()) + GAIN_BIAS;
}

bool InliningPass::is_inlining_beneficial(ir::Function *caller, ir::Function *callee) {
    if (call_graph.get_node(callee).preds.size() == 1) {
        return callee->get_basic_blocks().get_size() <= 64;
    }

    if (callee->get_basic_blocks().get_size() == 1) {
        return callee->get_entry_block_iter()->get_size() <= 64;
    }

    unsigned num_instrs = 0;
    for (ir::BasicBlock &block : *callee) {
        num_instrs += block.get_instrs().get_size();
    }

    return num_instrs <= 24;
}

bool InliningPass::is_inlining_legal(ir::Function *caller, ir::Function *callee) {
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
