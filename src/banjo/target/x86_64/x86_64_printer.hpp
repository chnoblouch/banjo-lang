#ifndef BANJO_TARGET_X86_64_PRINTER_H
#define BANJO_TARGET_X86_64_PRINTER_H

#include "banjo/mcode/printer.hpp"
#include "banjo/mcode/register.hpp"
#include "banjo/mcode/symbol.hpp"
#include "banjo/target/x86_64/x86_64_address.hpp"

namespace banjo::target {

class X8664Printer final : public mcode::Printer {

public:
    void print_opcode(mcode::Instruction &instr) override;
    void print_operand(mcode::Function &func, mcode::Instruction &instr, unsigned index) override;
    void print_physical_reg(mcode::PhysicalReg reg, unsigned size) override;

private:
    void print_register(mcode::Register reg, unsigned size);
    void print_address(mcode::Function &func, const X8664Address &addr);
    void print_symbol_deref(const mcode::Symbol &symbol);
    void print_size(unsigned size);
};

}; // namespace banjo::target

#endif
