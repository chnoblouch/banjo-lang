#include "dead_func_elimination_pass.hpp"
#include "banjo/ir/function.hpp"
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

    std::vector<ir::Function *> &funcs = mod.get_functions();
    std::vector<ir::Function *> new_funcs;

    for (unsigned i = 0; i < funcs.size(); i++) {
        if (funcs[i]->is_global() || used_funcs.count(funcs[i])) {
            new_funcs.push_back(funcs[i]);
        } else {
            delete funcs[i];
        }
    }

    mod.set_functions(new_funcs);
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
    } else if (value.is_branch_target()) {
        for (ir::Operand &arg : value.get_branch_target().args) {
            analyze_value(arg);
        }
    }
}

} // namespace passes

} // namespace banjo
