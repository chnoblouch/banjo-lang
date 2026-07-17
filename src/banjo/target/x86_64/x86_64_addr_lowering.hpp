#ifndef BANJO_TARGET_X86_64_ADDR_LOWERING_H
#define BANJO_TARGET_X86_64_ADDR_LOWERING_H

#include "banjo/mcode/operand.hpp"
#include "banjo/ssa/instruction.hpp"
#include "banjo/ssa/operand.hpp"

namespace banjo {

namespace target {

class X8664SSALowerer;

class X8664AddrLowering {

private:
    X8664SSALowerer &lowerer;

public:
    X8664AddrLowering(X8664SSALowerer &lowerer);

    mcode::Operand lower_address(const ssa::Operand &operand);
    mcode::Operand lower_reg_addr(ssa::VirtualRegister vreg);
    mcode::Operand lower_vreg_addr(mcode::Register reg);
    mcode::Operand lower_symbol_addr(const ssa::Operand &operand);

    X8664Address calc_offsetptr_addr(ssa::Instruction &instr);
    X8664Address calc_memberptr_addr(ssa::Instruction &instr);
};

} // namespace target

} // namespace banjo

#endif
