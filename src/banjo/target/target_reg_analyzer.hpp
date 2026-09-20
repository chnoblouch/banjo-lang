#ifndef BANJO_TARGET_REG_ANALYZER_H
#define BANJO_TARGET_REG_ANALYZER_H

#include "banjo/codegen/reg_alloc_func.hpp"
#include "banjo/mcode/instruction.hpp"
#include "banjo/mcode/register.hpp"
#include "banjo/mcode/stack_frame.hpp"
#include "banjo/ssa/virtual_register.hpp"

#include <vector>

namespace banjo::target {

struct InstrContext {
    mcode::Function &func;
    mcode::BasicBlockIter block;
    mcode::InstrIter instr;
};

struct SpilledRegUse {
    InstrContext &instr_ctx;
    mcode::StackSlotID stack_slot;
    mcode::PhysicalReg reg;
    codegen::RegClass reg_class;
};

class TargetRegAnalyzer {

public:
    virtual ~TargetRegAnalyzer() = default;
    virtual const std::vector<mcode::PhysicalReg> &get_candidates(codegen::RegClass reg_class) = 0;

    virtual void suggest_regs(
        codegen::RegAllocFunc &func,
        const codegen::Bundle &bundle,
        std::vector<mcode::PhysicalReg> &suggested_regs
    ) = 0;

    virtual bool is_reg_overridden(mcode::PhysicalReg reg, InstrContext &instr_ctx) = 0;
    virtual std::vector<mcode::RegOp> get_operands(InstrContext &instr_ctx) = 0;
    virtual void assign_reg_classes(mcode::Instruction &instr, codegen::RegClassMap &reg_classes) = 0;
    virtual bool is_move_from(mcode::Instruction &instr, ssa::VirtualRegister src_reg) { return false; }
    virtual void insert_load(SpilledRegUse use) = 0;
    virtual void insert_store(SpilledRegUse use) = 0;
    virtual bool is_instr_removable(mcode::Instruction &instr) = 0;
};

} // namespace banjo::target

#endif
