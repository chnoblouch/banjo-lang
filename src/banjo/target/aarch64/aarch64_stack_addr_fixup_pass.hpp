#ifndef BANJO_TARGET_AARCH64_STACK_ADDR_FIXUP_PASS_H
#define BANJO_TARGET_AARCH64_STACK_ADDR_FIXUP_PASS_H

#include "banjo/codegen/machine_pass.hpp"
#include "banjo/mcode/basic_block.hpp"

namespace banjo::target {

class AArch64StackAddrFixupPass final : public codegen::MachinePass {

private:
    mcode::BasicBlock *block;

public:
    void run(mcode::Module &mod);

private:
    void run(mcode::BasicBlock &block);
    void process_add_sub(mcode::InstrIter instr);
    void process_ldr_str(mcode::InstrIter instr, unsigned size);

    mcode::Operand emit_compute_stack_addr(mcode::InstrIter instr, unsigned offset);
    mcode::Operand emit_imm_mov(mcode::InstrIter instr, unsigned immediate);
};

} // namespace banjo::target

#endif
