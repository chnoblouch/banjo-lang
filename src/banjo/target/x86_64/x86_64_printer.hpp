#ifndef BANJO_TARGET_X86_64_PRINTER_H
#define BANJO_TARGET_X86_64_PRINTER_H

#include "banjo/mcode/printer.hpp"
#include "banjo/mcode/register.hpp"
#include "banjo/target/x86_64/x86_64_address.hpp"

namespace banjo::target {

class X8664Printer final : public mcode::Printer {

public:
    X8664Printer(std::string &buffer);

protected:
    void print_opcode(mcode::Instruction &instr) override;
    void print_operand(mcode::Instruction &instr, unsigned index) override;

private:
    void print_register(mcode::Register reg, unsigned size);
    void print_address(const X8664Address &address);
    void print_size(unsigned size);
};

}; // namespace banjo::target

#endif
