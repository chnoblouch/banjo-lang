#ifndef BANJO_TARGET_X86_64_PEEPHOLE_OPT_PASS_H
#define BANJO_TARGET_X86_64_PEEPHOLE_OPT_PASS_H

#include "banjo/codegen/machine_pass.hpp"
#include "banjo/mcode/instruction.hpp"

namespace banjo::target {

class X8664PeepholeOptPass : public codegen::MachinePass {

public:
    void run(mcode::Module &mod);

private:
    void run(mcode::Function &func);
    void run(mcode::Instruction &instr);

    void optimize_mov(mcode::Instruction &instr);
    void optimize_add(mcode::Instruction &instr);
    void optimize_sub(mcode::Instruction &instr);
    void optimize_and(mcode::Instruction &instr);
    void optimize_or(mcode::Instruction &instr);
    void optimize_xor(mcode::Instruction &instr);
    void optimize_movss(mcode::Instruction &instr);

    void optimize_encoding(mcode::Instruction &instr);
};

} // namespace banjo::target

#endif
