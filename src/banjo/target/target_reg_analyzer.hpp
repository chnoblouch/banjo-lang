#ifndef TARGET_REG_ANALYZER_H
#define TARGET_REG_ANALYZER_H

#include "banjo/codegen/reg_alloc_func.hpp"
#include "banjo/ssa/virtual_register.hpp"
#include "banjo/mcode/instruction.hpp"
#include "banjo/mcode/register.hpp"
#include "banjo/mcode/stack_frame.hpp"

#include <vector>

namespace banjo {

namespace target {

struct SpilledRegUse {
    mcode::InstrIter instr_iter;
    mcode::BasicBlock &block;
    mcode::StackSlotID stack_slot;
    mcode::PhysicalReg reg;
};

class TargetRegAnalyzer {

public:
    virtual ~TargetRegAnalyzer() = default;
    virtual const std::vector<mcode::PhysicalReg> &get_candidates(codegen::RegClass reg_class) = 0;

    virtual std::vector<mcode::PhysicalReg> suggest_regs(
        codegen::RegAllocFunc &func,
        const codegen::Bundle &bundle
    ) = 0;

    virtual bool is_reg_overridden(
        mcode::Instruction &instr,
        mcode::BasicBlock &basic_block,
        mcode::PhysicalReg reg
    ) = 0;

    virtual std::vector<mcode::RegOp> get_operands(mcode::InstrIter iter, mcode::BasicBlock &block) = 0;
    virtual void assign_reg_classes(mcode::Instruction &instr, codegen::RegClassMap &reg_classes) = 0;
    virtual bool is_move_from(mcode::Instruction &instr, ssa::VirtualRegister src_reg) { return false; }
    virtual void insert_load(SpilledRegUse use) = 0;
    virtual void insert_store(SpilledRegUse use) = 0;
    virtual bool is_instr_removable(mcode::Instruction &instr) = 0;
};

} // namespace target

} // namespace banjo

#endif
