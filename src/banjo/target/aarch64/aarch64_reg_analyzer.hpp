#ifndef AARCH64_REG_ANALYZER_H
#define AARCH64_REG_ANALYZER_H

#include "target/aarch64/aarch64_register.hpp"
#include "target/target_reg_analyzer.hpp"

namespace target {

class AArch64RegAnalyzer : public TargetRegAnalyzer {

private:
    static constexpr int NUM_REGS = AArch64Register::V_LAST + 1;

    std::vector<int> general_purpose_regs;
    std::vector<int> float_regs;

public:
    AArch64RegAnalyzer();

    std::vector<int> get_all_registers() override;
    const std::vector<int> &get_candidates(mcode::Instruction &instr) override;
    std::vector<mcode::PhysicalReg> suggest_regs(codegen::RegAllocFunc &func, const codegen::LiveRangeGroup &group)
        override;
    bool is_reg_overridden(mcode::Instruction &instr, mcode::BasicBlock &basic_block, int reg) override;
    std::vector<mcode::RegOp> get_operands(mcode::InstrIter iter, mcode::BasicBlock &block) override;
    mcode::PhysicalReg insert_spill_reload(SpilledRegUse use) override;
    bool is_instr_removable(mcode::Instruction &instr) override;

private:
    bool is_move_opcode(mcode::Opcode opcode);
    bool is_float_instr(mcode::Instruction &instr);
    void collect_regs(mcode::Operand &operand, mcode::RegUsage usage, std::vector<mcode::RegOp> &dst);
    void collect_addr_regs(mcode::Operand &operand, std::vector<mcode::RegOp> &dst);
};

} // namespace target

#endif
