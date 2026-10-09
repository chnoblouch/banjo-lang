#ifndef BANJO_PASSES_STACK_TO_REG_PASS_H
#define BANJO_PASSES_STACK_TO_REG_PASS_H

#include "banjo/passes/pass.hpp"
#include "banjo/ssa/control_flow_graph.hpp"
#include "banjo/ssa/function.hpp"
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
    ssa::DominatorTree dom_tree;

    StackSlotMap slots;
    BlockMap blocks;

public:
    StackToRegPass(target::Target *target);
    void run(ssa::Module &mod);

private:
    void run(ssa::Function &func);

    void find_stack_slots(ssa::Instruction &instr);
    void find_slot_uses(NodeID node, ssa::Instruction &instr);
    void analyze_reg_use(ssa::VirtualRegister reg, NodeID node, ssa::Opcode opcode);
    bool is_slot_loaded(StackSlotInfo &slot, NodeID node, BitSet &nodes_visited);

    void try_promote(ssa::VirtualRegister reg, StackSlotInfo &slot, ValueMap &initial_values);

    void update_uses(ssa::BasicBlockIter block_iter, ValueMap current_values);
    void update_uses_in_load(ssa::BasicBlock &block, ssa::InstrIter &instr, ValueMap &current_values);
    void update_uses_in_store(ssa::BasicBlock &block, ssa::InstrIter &instr, ValueMap &current_values);

    void replace_regs(std::vector<ssa::Operand> &operands, ValueMap current_values);
    void update_branch_target(ssa::Operand &operand, ValueMap current_values);

    void print_dump();
};

} // namespace banjo::passes

#endif
