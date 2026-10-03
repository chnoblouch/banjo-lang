#ifndef BANJO_MCODE_PRINTER_H
#define BANJO_MCODE_PRINTER_H

#include "banjo/mcode/instruction.hpp"
#include "banjo/mcode/module.hpp"
#include "banjo/mcode/register.hpp"

#include <string>
#include <string_view>

namespace banjo::mcode {

class Printer {

private:
    std::string &buffer;

public:
    Printer(std::string &buffer);
    virtual ~Printer() = default;

    void print(mcode::Module &mod);

private:
    void print_func(mcode::Function &func);

protected:
    virtual void print_opcode(mcode::Instruction &instr) = 0;
    virtual void print_operand(mcode::Instruction &instr, unsigned index) = 0;

    void print_virtual_reg(mcode::VirtualReg reg, unsigned size);

    void emit(std::string_view value);
    void emit(char c);
    void emit(int value);
    void emit(unsigned value);
    void emit(long long value);
    void emit(unsigned long long value);
};

} // namespace banjo::mcode

#endif
