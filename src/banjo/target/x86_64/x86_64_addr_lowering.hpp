#ifndef X86_64_ADDR_LOWERING_H
#define X86_64_ADDR_LOWERING_H

#include "banjo/ir/instruction.hpp"
#include "banjo/ir/operand.hpp"
#include "banjo/mcode/operand.hpp"

namespace banjo {

namespace target {

class X8664IRLowerer;

class X8664AddrLowering {

private:
    X8664IRLowerer &lowerer;

public:
    X8664AddrLowering(X8664IRLowerer &lowerer);

    mcode::Operand lower_address(const ir::Operand &operand);
    mcode::Operand lower_reg_addr(ir::VirtualRegister vreg);
    mcode::Operand lower_vreg_addr(mcode::Register reg);
    mcode::Operand lower_symbol_addr(const ir::Operand &operand);

    mcode::IndirectAddress calc_offsetptr_addr(ir::Instruction &instr);
    mcode::IndirectAddress calc_memberptr_addr(ir::Instruction &instr);

private:
};

} // namespace target

} // namespace banjo

#endif
