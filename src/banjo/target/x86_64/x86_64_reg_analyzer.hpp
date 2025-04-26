#ifndef X86_64_REG_ANALYZER_H
#define X86_64_REG_ANALYZER_H

#include "banjo/codegen/reg_alloc_func.hpp"
#include "banjo/mcode/instruction.hpp"
#include "banjo/target/target_reg_analyzer.hpp"
#include "banjo/target/x86_64/x86_64_register.hpp"

namespace banjo {

namespace target {

namespace X8664RegClass {
enum {
    GENERAL_PURPOSE,
    SSE,
};
} // namespace X8664RegClass

class X8664RegAnalyzer : public TargetRegAnalyzer {

    static constexpr unsigned NUM_REGS = X8664Register::XMM15 + 1;

    std::vector<mcode::PhysicalReg> general_purpose_regs;
    std::vector<mcode::PhysicalReg> sse_regs;

public:
    X8664RegAnalyzer();

    const std::vector<mcode::PhysicalReg> &get_candidates(codegen::RegClass reg_class) override;
    std::vector<mcode::PhysicalReg> suggest_regs(codegen::RegAllocFunc &func, const codegen::Bundle &bundle) override;
    bool is_reg_overridden(mcode::Instruction &instr, mcode::BasicBlock &basic_block, mcode::PhysicalReg reg) override;
    std::vector<mcode::RegOp> get_operands(mcode::InstrIter iter, mcode::BasicBlock &block) override;
    void assign_reg_classes(mcode::Instruction &instr, codegen::RegClassMap &reg_classes) override;
    bool is_move_from(mcode::Instruction &instr, ssa::VirtualRegister src_reg) override;
    void insert_load(SpilledRegUse use) override;
    void insert_store(SpilledRegUse use) override;
    bool is_instr_removable(mcode::Instruction &instr) override;

private:
    bool is_move_opcode(mcode::Opcode opcode);
    bool is_memory_operand_allowed(mcode::Instruction &instr);
    mcode::Opcode get_move_opcode(codegen::RegClass reg_class, unsigned size);

    void collect_regs(mcode::Operand &operand, mcode::RegUsage usage, std::vector<mcode::RegOp> &dst);
    void collect_addr_regs(mcode::Operand &operand, std::vector<mcode::RegOp> &dst);

};

} // namespace target

} // namespace banjo

#endif
