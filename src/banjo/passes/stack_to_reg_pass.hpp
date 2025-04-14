#ifndef PASSES_STACK_TO_REG_PASS_H
#define PASSES_STACK_TO_REG_PASS_H

#include "banjo/passes/pass.hpp"
#include "banjo/ssa/control_flow_graph.hpp"

#include <list>
#include <unordered_map>
#include <unordered_set>

namespace banjo {

namespace passes {

class StackToRegPass : public Pass {

private:
    struct StackSlotInfo {
        ssa::Type type;
        std::list<ssa::BasicBlockIter> store_blocks;
        std::unordered_set<ssa::BasicBlockIter> load_blocks;
        bool promotable;
        ssa::Value cur_replacement;
        std::unordered_set<ssa::BasicBlockIter> blocks_having_val_as_param;
    };

    struct ParamInfo {
        unsigned param_index;
        ssa::VirtualRegister stack_slot;
    };

    struct BlockInfo {
        std::vector<ParamInfo> new_params;
    };

    typedef std::unordered_map<long, StackSlotInfo> StackSlotMap;
    typedef std::unordered_map<ssa::BasicBlockIter, BlockInfo> BlockMap;

public:
    StackToRegPass(target::Target *target);
    void run(ssa::Module &mod);

private:
    void run(ssa::Function *func);
    StackSlotMap find_stack_slots(ssa::Function *func);
    void find_slot_uses(StackSlotMap &slots, ssa::BasicBlockIter block, ssa::Instruction &instr);
    void analyze_reg_use(StackSlotMap &slots, ssa::VirtualRegister reg, ssa::BasicBlockIter block, ssa::Opcode opcode);

    bool is_slot_loaded(
        StackSlotInfo &slot,
        ssa::ControlFlowGraph &cfg,
        unsigned node_index,
        std::unordered_set<unsigned> &nodes_visited
    );

    ssa::Value create_undefined(ssa::Type type);

    void rename(
        ssa::BasicBlockIter block_iter,
        StackSlotMap &slots,
        BlockMap &blocks,
        std::unordered_map<long, ssa::Value> cur_replacements,
        ssa::DominatorTree &dominator_tree
    );

    void replace_regs(std::vector<ssa::Operand> &operands, std::unordered_map<long, ssa::Value> cur_replacements);

    void update_branch_target(
        ssa::Operand &operand,
        BlockMap &blocks,
        std::unordered_map<long, ssa::Value> cur_replacements
    );
};

} // namespace passes

} // namespace banjo

#endif
