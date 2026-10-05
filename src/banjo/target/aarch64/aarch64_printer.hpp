#ifndef BANJO_TARGET_AARCH64_PRINTER_H
#define BANJO_TARGET_AARCH64_PRINTER_H

#include "banjo/mcode/printer.hpp"
#include "banjo/mcode/register.hpp"
#include "banjo/target/aarch64/aarch64_address.hpp"

namespace banjo::target {

class AArch64Printer final : public mcode::Printer {

public:
    using mcode::Printer::Printer;

private:
    void print_opcode(mcode::Instruction &instr) override;
    void print_operand(mcode::Instruction &instr, unsigned index) override;

    void print_register(mcode::Register reg, unsigned size);
    void print_address(const AArch64Address &address);
};

}; // namespace banjo::target

#endif
