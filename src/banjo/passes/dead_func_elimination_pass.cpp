#include "dead_func_elimination_pass.hpp"

#include "banjo/ir/function.hpp"
#include "banjo/ir/function_decl.hpp"
#include <vector>

namespace banjo {

namespace passes {

DeadFuncEliminationPass::DeadFuncEliminationPass(target::Target *target) : Pass("dead-func-elimination", target) {}

void DeadFuncEliminationPass::run(ir::Module &mod) {
    this->mod = &mod;

    std::vector<ir::Function *> roots;

    for (ir::Function *func : mod.get_functions()) {
        if (func->is_global()) {
            roots.push_back(func);
        }
    }

    for (ir::Global &global : mod.get_globals()) {
        if (global.get_initial_value().is_func()) {
            roots.push_back(global.get_initial_value().get_func());
        }
    }

    for (ir::Function *root : roots) {
        walk_call_graph(root);
    }

    std::vector<ir::Function *> new_funcs;

    for (ir::Function *func : mod.get_functions()) {
        if (func->is_global() || used_funcs.contains(func)) {
            new_funcs.push_back(func);
        } else {
            delete func;
        }
    }

    mod.set_functions(new_funcs);

    std::vector<ir::FunctionDecl> new_extern_funcs;

    for (ir::FunctionDecl &extern_func : mod.get_external_functions()) {
        if (used_extern_funcs.contains(extern_func.get_name())) {
            new_extern_funcs.push_back(extern_func);
        }
    }

    mod.set_external_functions(new_extern_funcs);
}

void DeadFuncEliminationPass::walk_call_graph(ir::Function *func) {
    used_funcs.insert(func);

    for (ir::BasicBlock &basic_block : func->get_basic_blocks()) {
        for (ir::Instruction &instr : basic_block.get_instrs()) {
            for (ir::Operand &operand : instr.get_operands()) {
                analyze_value(operand);
            }
        }
    }
}

void DeadFuncEliminationPass::analyze_value(ir::Value &value) {
    if (value.is_func()) {
        if (!used_funcs.contains(value.get_func())) {
            walk_call_graph(value.get_func());
        }
    } else if (value.is_extern_func()) {
        used_extern_funcs.insert(value.get_extern_func_name());
    } else if (value.is_branch_target()) {
        for (ir::Operand &arg : value.get_branch_target().args) {
            analyze_value(arg);
        }
    }
}

} // namespace passes

} // namespace banjo
