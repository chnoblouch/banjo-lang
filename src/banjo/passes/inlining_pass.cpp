#include "inlining_pass.hpp"

#include "banjo/passes/pass_utils.hpp"
#include "banjo/passes/precomputing.hpp"
#include "banjo/ssa/basic_block.hpp"
#include "banjo/ssa/operand.hpp"

#include <string>
#include <vector>

#define DEBUG_LOG is_logging() && log()

namespace banjo::passes {

constexpr int GAIN_BIAS = 3;

InliningPass::InliningPass(target::Target *target) : Pass("inlining", target) {
    // enable_logging(std::cout);
}

void InliningPass::run(ssa::Module &mod) {
    this->mod = &mod;
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

void InliningPass::inline_func(ssa::Function &func, ssa::BasicBlockIter &block_iter, ssa::InstrIter &call_iter) {
    ssa::BasicBlock &block = *block_iter;
    ssa::Instruction &call_instr = *call_iter;

    std::vector<ssa::Operand> call_operands = call_instr.get_operands();
    ssa::Function &callee = *call_instr.get_operand(0).get_func();

    Context ctx{
        .caller = func,
        .call_instr = call_iter,
        .end_block = func.split_block_after(block_iter, call_iter, mod->next_block_label()),
    };

    if (call_instr.get_dest()) {
        ctx.end_block->param_regs.push_back(*call_instr.get_dest());
        ctx.end_block->param_types.push_back(call_instr.get_operand(0).get_type());
    }

    for (ssa::BasicBlock &callee_block : callee) {
        for (ssa::InstrIter instr = callee_block.begin(); instr != callee_block.end(); ++instr) {
            if (instr->get_dest()) {
                ctx.reg2reg.insert({*instr->get_dest(), func.next_virtual_reg()});
            }
        }
    }

    for (ssa::BasicBlockIter iter = callee.begin(); iter != callee.end(); ++iter) {
        ssa::BasicBlockIter inline_block = func.insert_before(ctx.end_block, mod->next_block_label());
        ctx.block_map.insert({iter, inline_block});

        inline_block->param_regs.resize(iter->param_regs.size());
        inline_block->param_types.resize(iter->param_regs.size());

        for (unsigned i = 0; i < iter->param_regs.size(); i++) {
            ssa::VirtualRegister inline_param_reg = func.next_virtual_reg();
            inline_block->param_regs[i] = inline_param_reg;
            inline_block->param_types[i] = iter->param_types[i];
            ctx.reg2reg.insert({iter->param_regs[i], inline_param_reg});
        }
    }

    for (ssa::BasicBlockIter block = callee.begin(); block != callee.end(); ++block) {
        ssa::BasicBlockIter inline_block = ctx.block_map[block];

        for (ssa::Instruction &instr : block->instrs) {
            inline_instr(instr, *inline_block, ctx);
        }
    }

    ssa::BranchTarget target{.block = block_iter.get_next(), .args{}};

    for (unsigned i = 1; i < call_iter->get_operands().size(); i++) {
        target.args.push_back(call_iter->get_operand(i));
    }

    block.replace(call_iter, {ssa::Opcode::JMP, {ssa::Operand::from_branch_target(target)}});

    block_iter = ctx.end_block;
    call_iter = ctx.end_block->get_instrs().get_first_iter().get_prev();
}

void InliningPass::inline_instr(ssa::Instruction instr, ssa::BasicBlock &block, Context &ctx) {
    if (instr.get_opcode() == ssa::Opcode::RET) {
        ssa::BranchTarget target{.block = ctx.end_block, .args{}};

        if (!ctx.end_block->param_regs.empty()) {
            target.args.push_back(get_inlined_value(instr.get_operand(0), ctx));
        }

        instr = {ssa::Opcode::JMP, {ssa::Operand::from_branch_target(target)}};
    }

    if (instr.get_dest()) {
        auto dst_reg2reg_iter = ctx.reg2reg.find(*instr.get_dest());
        if (dst_reg2reg_iter != ctx.reg2reg.end()) {
            instr.set_dest(dst_reg2reg_iter->second);
        }
    }

    for (ssa::Operand &operand : instr.get_operands()) {
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

    if (instr.get_opcode() == ssa::Opcode::ALLOCA) {
        ssa::BasicBlock &entry_block = ctx.caller.basic_blocks.get_first();
        entry_block.insert_before(entry_block.get_entry_iter(), instr);
    } else {
        block.append(instr);
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

    return num_instrs <= 48;
}

bool InliningPass::is_inlining_legal(ssa::Function *caller, ssa::Function *callee) {
    if (callee->never_inline) {
        return false;
    }

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

} // namespace banjo::passes
