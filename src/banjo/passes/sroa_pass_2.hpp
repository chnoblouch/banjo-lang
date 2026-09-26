#ifndef BANJO_PASSES_SROA_PASS_2_H
#define BANJO_PASSES_SROA_PASS_2_H

#include "banjo/passes/pass.hpp"
#include "banjo/ssa/basic_block.hpp"
#include "banjo/ssa/instruction.hpp"
#include "banjo/ssa/primitive.hpp"
#include "banjo/ssa/virtual_register.hpp"
#include "banjo/utils/hash_map.hpp"

#include <optional>
#include <vector>

namespace banjo::passes {

class SROAPass2 : public Pass {

private:
    struct InstrContext {
        ssa::InstrIter instr;
        ssa::BasicBlock &block;
    };

    struct Member {
        unsigned offset;
        ssa::Type type;
        std::optional<ssa::VirtualRegister> replacement;
    };

    struct StackSlot {
        InstrContext alloca;
        std::vector<Member> members;
        bool splittable;
    };

    struct StackReference {
        unsigned slot_index;
        unsigned member_index;
    };

    struct StackRefInstr {
        InstrContext context;
        StackReference reference;
    };

    struct StackAccess {
        ssa::Operand &operand;
        StackReference reference;
    };

    target::TargetDataLayout &data_layout;

    std::vector<StackSlot> stack_slots;
    HashMap<ssa::VirtualRegister, StackReference> stack_refs;
    std::vector<StackRefInstr> stack_ref_instrs;
    std::vector<StackAccess> stack_accesses;

public:
    SROAPass2(target::Target *target);
    void run(ssa::Module &mod);

private:
    void run(ssa::Function &func);

    void collect_stack_slots(ssa::BasicBlock &block);
    bool collect_members(StackSlot &slot, ssa::Structure &struct_, unsigned base_offset);

    void collect_stack_refs(ssa::BasicBlock &block);
    void collect_memberptr(ssa::InstrIter instr, ssa::BasicBlock &block);
    void collect_offsetptr(ssa::InstrIter instr, ssa::BasicBlock &block);
    void collect_load(ssa::Instruction &instr);
    void collect_store(ssa::Instruction &instr);
    void analyze_sub_pointer(InstrContext context, ssa::VirtualRegister base, int offset);

    void dump(ssa::Function &func);
    void dump_primitive(ssa::Primitive primitive);
};

} // namespace banjo::passes

#endif
