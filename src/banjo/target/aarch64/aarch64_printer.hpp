#ifndef BANJO_TARGET_AARCH64_PRINTER_H
#define BANJO_TARGET_AARCH64_PRINTER_H

#include "banjo/mcode/printer.hpp"
#include "banjo/mcode/register.hpp"
#include "banjo/target/aarch64/aarch64_address.hpp"
#include "banjo/target/aarch64/aarch64_condition.hpp"

namespace banjo::target {

class AArch64Printer final : public mcode::Printer {

public:
    void print_opcode(mcode::Instruction &instr) override;
    void print_operand(mcode::Function &func, mcode::Instruction &instr, unsigned index) override;
    void print_physical_reg(mcode::PhysicalReg reg, unsigned size) override;

private:
    void print_register(mcode::Register reg, unsigned size);
    void print_address(mcode::Function &func, const AArch64Address &addr);
    void print_left_shift(unsigned shift);
    void print_condition(AArch64Condition condition);
};

}; // namespace banjo::target

#endif
