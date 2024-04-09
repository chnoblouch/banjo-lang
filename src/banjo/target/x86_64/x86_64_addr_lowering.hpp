#ifndef X86_64_ADDR_LOWERING_H
#define X86_64_ADDR_LOWERING_H

#include "codegen/ir_lowerer.hpp"

namespace target {

class X8664AddrLowering {

private:
    codegen::IRLowerer &lowerer;

public:
    X8664AddrLowering(codegen::IRLowerer &lowerer);

    mcode::Operand lower_address(const ir::Operand &operand);
    mcode::Operand lower_reg_addr(ir::VirtualRegister vreg);
    mcode::Operand lower_vreg_addr(mcode::Register reg);
    mcode::Operand lower_symbol_addr(const ir::Operand &operand);

    mcode::IndirectAddress calc_offsetptr_addr(ir::Instruction &instr);
    mcode::IndirectAddress calc_memberptr_addr(ir::Instruction &instr);

private:
};

} // namespace target

#endif
