#ifndef BANJO_PASSES_CONTROL_FLOW_OPT_PASS_H
#define BANJO_PASSES_CONTROL_FLOW_OPT_PASS_H

#include "banjo/passes/pass.hpp"
#include "banjo/ssa/basic_block.hpp"
#include "banjo/ssa/control_flow_graph.hpp"
#include "banjo/ssa/operand.hpp"
#include "banjo/ssa/virtual_register.hpp"

#include <unordered_map>
#include <unordered_set>

namespace banjo::passes {

class ControlFlowOptPass : public Pass {

private:
    ssa::Function *func;
    ssa::DominatorTree *dom_tree;

    std::unordered_map<ssa::VirtualRegister, ssa::BasicBlockIter> arg_blocks;
    std::unordered_set<ssa::BasicBlockIter> blocks_with_escaping_args;

public:
    ControlFlowOptPass(target::Target *target);
    void run(ssa::Module &mod);

private:
    void run(ssa::Function &func);
    void run_iteration(ssa::Function &func);

    void try_inline_into_jmp(ssa::BasicBlockIter origin_block, ssa::Instruction &branch_instr);
    void try_inline_into_cjmp(ssa::BasicBlockIter origin_block, ssa::BranchTarget &target);
    bool try_replace_regs_with_block_args(ssa::Value &value, ssa::BranchTarget &target, ssa::BasicBlockIter block);
    bool is_reg_always_defined(ssa::VirtualRegister reg, ssa::BasicBlockIter block);
};

} // namespace banjo::passes

#endif
