#ifndef AARCH64_REG_ANALYZER_H
#define AARCH64_REG_ANALYZER_H

#include "banjo/mcode/instruction.hpp"
#include "banjo/target/aarch64/aarch64_register.hpp"
#include "banjo/target/target_reg_analyzer.hpp"

namespace banjo {

namespace target {

namespace AArch64RegClass {
enum AArch64RegClass {
    GENERAL_PURPOSE,
    FP_AND_VECTOR,
};
} // namespace AArch64RegClass

class AArch64RegAnalyzer : public TargetRegAnalyzer {

    static constexpr unsigned NUM_REGS = AArch64Register::V_LAST + 1;

    std::vector<mcode::PhysicalReg> general_purpose_regs;
    std::vector<mcode::PhysicalReg> fp_and_vector_regs;

public:
    AArch64RegAnalyzer();

    std::vector<mcode::PhysicalReg> get_all_registers() override;
    const std::vector<mcode::PhysicalReg> &get_candidates(codegen::RegClass reg_class) override;
    std::vector<mcode::PhysicalReg> suggest_regs(codegen::RegAllocFunc &func, const codegen::Bundle &bundle) override;
    bool is_reg_overridden(mcode::Instruction &instr, mcode::BasicBlock &basic_block, mcode::PhysicalReg reg) override;
    std::vector<mcode::RegOp> get_operands(mcode::InstrIter iter, mcode::BasicBlock &block) override;
    void assign_reg_classes(mcode::Instruction &instr, codegen::RegClassMap &reg_classes) override;
    void insert_load(SpilledRegUse use) override {}
    void insert_store(SpilledRegUse use) override {}
    bool is_instr_removable(mcode::Instruction &instr) override;

private:
    bool is_move_opcode(mcode::Opcode opcode);
    bool is_float_instr(mcode::Instruction &instr);
    void collect_regs(mcode::Operand &operand, mcode::RegUsage usage, std::vector<mcode::RegOp> &dst);
    void collect_addr_regs(mcode::Operand &operand, std::vector<mcode::RegOp> &dst);
};

} // namespace target

} // namespace banjo

#endif
