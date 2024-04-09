#ifndef TARGET_REG_ANALYZER_H
#define TARGET_REG_ANALYZER_H

#include "codegen/liveness.hpp"
#include "codegen/reg_alloc_func.hpp"

#include <vector>

namespace target {

struct SpilledRegUse {
    mcode::InstrIter instr_iter;
    mcode::BasicBlock &block;
    long stack_slot;
    unsigned spill_tmp_regs;
    mcode::RegUsage usage;
};

class TargetRegAnalyzer {

public:
    virtual ~TargetRegAnalyzer() = default;
    virtual std::vector<int> get_all_registers() = 0;
    virtual const std::vector<int> &get_candidates(mcode::Instruction &instr) = 0;

    virtual std::vector<mcode::PhysicalReg> suggest_regs(
        codegen::RegAllocFunc &func,
        const codegen::LiveRangeGroup &group
    ) = 0;

    virtual bool is_reg_overridden(mcode::Instruction &instr, mcode::BasicBlock &basic_block, int reg) = 0;
    virtual std::vector<mcode::RegOp> get_operands(mcode::InstrIter iter, mcode::BasicBlock &block) = 0;
    virtual mcode::PhysicalReg insert_spill_reload(SpilledRegUse use) = 0;
    virtual bool is_instr_removable(mcode::Instruction &instr) = 0;
};

} // namespace target

#endif
