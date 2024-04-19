#ifndef PASSES_STACK_TO_REG_PASS_H
#define PASSES_STACK_TO_REG_PASS_H

#include "ir/control_flow_graph.hpp"
#include "passes/pass.hpp"

#include <list>
#include <unordered_map>
#include <unordered_set>

namespace passes {

class StackToRegPass : public Pass {

private:
    struct StackSlotInfo {
        ir::Type type;
        std::list<ir::BasicBlockIter> def_blocks;
        std::unordered_set<ir::BasicBlockIter> use_blocks;
        bool promotable;
        ir::Value cur_replacement;
        std::unordered_set<ir::BasicBlockIter> blocks_having_val_as_param;
    };

    struct ParamInfo {
        unsigned param_index;
        ir::VirtualRegister stack_slot;
    };

    struct BlockInfo {
        std::vector<ParamInfo> new_params;
    };

    typedef std::unordered_map<long, StackSlotInfo> StackSlotMap;
    typedef std::unordered_map<ir::BasicBlockIter, BlockInfo> BlockMap;

public:
    StackToRegPass(target::Target *target);
    void run(ir::Module &mod);

private:
    void run(ir::Function *func, ir::Module &mod);
    StackSlotMap find_stack_slots(ir::Function *func, ir::Module &mod);

    bool is_stack_slot_used(
        StackSlotInfo &stack_slot,
        ir::ControlFlowGraph &cfg,
        unsigned node_index,
        std::unordered_set<unsigned> &nodes_visited
    );

    void rename(
        ir::BasicBlockIter block_iter,
        StackSlotMap &stack_slots,
        BlockMap &blocks,
        std::unordered_map<long, ir::Value> cur_replacements,
        ir::DominatorTree &dominator_tree
    );

    void replace_regs(std::vector<ir::Operand> &operands, std::unordered_map<long, ir::Value> cur_replacements);

    void update_branch_target(
        ir::Operand &operand,
        BlockMap &blocks,
        std::unordered_map<long, ir::Value> cur_replacements
    );
};

} // namespace passes

#endif
