#ifndef X86_64_REG_ANALYZER_H
#define X86_64_REG_ANALYZER_H

#include "target/target_reg_analyzer.hpp"
#include "target/x86_64/x86_64_register.hpp"

namespace target {

class X8664RegAnalyzer : public TargetRegAnalyzer {

private:
    static constexpr int NUM_REGS = X8664Register::XMM15 + 1;

    std::vector<int> general_purpose_regs;
    std::vector<int> float_regs;

public:
    X8664RegAnalyzer();

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
    bool is_float_operand(mcode::Opcode opcode, mcode::RegUsage usage);

    void add_du_ops(mcode::Instruction &instr, std::vector<mcode::RegOp> &dst);
    void add_udu_ops(mcode::Instruction &instr, std::vector<mcode::RegOp> &dst);
    void collect_regs(mcode::Operand &operand, mcode::RegUsage usage, std::vector<mcode::RegOp> &dst);
    void collect_addr_regs(mcode::Operand &operand, std::vector<mcode::RegOp> &dst);
};

} // namespace target

#endif
