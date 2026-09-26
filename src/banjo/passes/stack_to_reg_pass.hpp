#ifndef BANJO_PASSES_STACK_TO_REG_PASS_H
#define BANJO_PASSES_STACK_TO_REG_PASS_H

#include "banjo/passes/pass.hpp"
#include "banjo/ssa/control_flow_graph.hpp"
#include "banjo/ssa/virtual_register.hpp"
#include "banjo/target/target_data_layout.hpp"
#include "banjo/utils/bit_set.hpp"

#include <unordered_map>

namespace banjo::passes {

class StackToRegPass : public Pass {

private:
    typedef ssa::ControlFlowGraph::NodeID NodeID;

    struct StackSlotInfo {
        ssa::Type type;
        BitSet store_nodes;
        BitSet load_nodes;
        BitSet nodes_having_val_as_param;
        bool promotable;
        ssa::Value cur_replacement;
    };

    struct ParamInfo {
        unsigned param_index;
        ssa::VirtualRegister stack_slot;
    };

    struct BlockInfo {
        std::vector<ParamInfo> new_params;
    };

    typedef std::unordered_map<ssa::VirtualRegister, StackSlotInfo> StackSlotMap;
    typedef std::unordered_map<ssa::BasicBlockIter, BlockInfo> BlockMap;
    typedef std::unordered_map<ssa::VirtualRegister, ssa::Value> ValueMap;

    target::TargetDataLayout &data_layout;

    ssa::Function *func;
    ssa::ControlFlowGraph cfg;
    ssa::DominatorTree domtree;

public:
    StackToRegPass(target::Target *target);
    void run(ssa::Module &mod);

private:
    void run(ssa::Function &func);
    StackSlotMap find_stack_slots();
    void find_slot_uses(StackSlotMap &slots, NodeID node, ssa::Instruction &instr);
    void analyze_reg_use(StackSlotMap &slots, ssa::VirtualRegister reg, NodeID node, ssa::Opcode opcode);
    bool is_slot_loaded(StackSlotInfo &slot, NodeID node, BitSet &nodes_visited);

    void rename(ssa::BasicBlockIter block_iter, StackSlotMap &slots, BlockMap &blocks, ValueMap cur_replacements);
    void rename_in_load(ssa::BasicBlock &block, ssa::InstrIter &instr, StackSlotMap &slots, ValueMap &cur_replacements);
    void rename_in_store(
        ssa::BasicBlock &block,
        ssa::InstrIter &instr,
        StackSlotMap &slots,
        ValueMap &cur_replacements
    );

    void replace_regs(std::vector<ssa::Operand> &operands, ValueMap cur_replacements);
    void update_branch_target(ssa::Operand &operand, BlockMap &blocks, ValueMap cur_replacements);
};

} // namespace banjo::passes

#endif
