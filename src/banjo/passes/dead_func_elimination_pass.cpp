#include "dead_func_elimination_pass.hpp"

#include "banjo/ssa/function.hpp"

#include <vector>

namespace banjo {

namespace passes {

DeadFuncEliminationPass::DeadFuncEliminationPass(target::Target *target) : Pass("dead-func-elimination", target) {}

void DeadFuncEliminationPass::run(ssa::Module &mod) {
    this->mod = &mod;

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
        walk_call_graph(root);
    }

    std::vector<ssa::Function *> new_funcs;

    for (ssa::Function *func : mod.get_functions()) {
        if (func->global || used_funcs.contains(func)) {
            new_funcs.push_back(func);
        } else {
            delete func;
        }
    }

    mod.set_functions(new_funcs);

    std::vector<ssa::FunctionDecl *> new_extern_funcs;

    used_extern_funcs.insert({"memcpy"});
    used_extern_funcs.insert({"sqrt"});

    for (ssa::FunctionDecl *extern_func : mod.get_external_functions()) {
        if (used_extern_funcs.contains(extern_func->name)) {
            new_extern_funcs.push_back(extern_func);
        }
    }

    mod.set_external_functions(new_extern_funcs);
}

void DeadFuncEliminationPass::walk_call_graph(ssa::Function *func) {
    used_funcs.insert(func);

    for (ssa::BasicBlock &basic_block : func->get_basic_blocks()) {
        for (ssa::Instruction &instr : basic_block.get_instrs()) {
            for (ssa::Operand &operand : instr.get_operands()) {
                analyze_value(operand);
            }
        }
    }
}

void DeadFuncEliminationPass::analyze_value(ssa::Value &value) {
    if (value.is_func()) {
        if (!used_funcs.contains(value.get_func())) {
            walk_call_graph(value.get_func());
        }
    } else if (value.is_extern_func()) {
        used_extern_funcs.insert(value.get_extern_func()->name);
    } else if (value.is_branch_target()) {
        for (ssa::Operand &arg : value.get_branch_target().args) {
            analyze_value(arg);
        }
    }
}

} // namespace passes

} // namespace banjo
