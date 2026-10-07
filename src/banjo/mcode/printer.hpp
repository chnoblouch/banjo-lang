#ifndef BANJO_MCODE_PRINTER_H
#define BANJO_MCODE_PRINTER_H

#include "banjo/mcode/function.hpp"
#include "banjo/mcode/global.hpp"
#include "banjo/mcode/instruction.hpp"
#include "banjo/mcode/module.hpp"
#include "banjo/mcode/register.hpp"
#include "banjo/mcode/stack_address.hpp"
#include "banjo/mcode/symbol.hpp"

#include <string>
#include <string_view>

namespace banjo::mcode {

class Printer {

public:
    static constexpr unsigned NO_ATTRIBUTES = 0x00000001;

private:
    std::string *buffer;
    unsigned flags;

public:
    virtual ~Printer() = default;

    Printer &set_buffer(std::string &buffer);
    Printer &set_flags(unsigned flags);

    void print(mcode::Module &mod);
    void print_external(std::string_view name);
    void print_global(std::string_view name);
    void print_data(mcode::Global &global);
    void print_func(mcode::Function &func);
    void print_stack_slot(mcode::StackSlotID id, mcode::StackSlot &slot);
    void print_instr(mcode::Function &func, mcode::Instruction &instr);

    virtual void print_opcode(mcode::Instruction &instr) = 0;
    virtual void print_operand(mcode::Function &func, mcode::Instruction &instr, unsigned index) = 0;
    virtual void print_physical_reg(mcode::PhysicalReg reg, unsigned size) = 0;

protected:
    void print_register(mcode::Register reg, unsigned size);
    void print_virtual_reg(mcode::VirtualReg reg, unsigned size);
    void print_stack_slot(mcode::StackSlotID slot);
    void print_stack_addr(mcode::Function &func, const mcode::StackAddress &stack_addr);
    void print_symbol(const mcode::Symbol &symbol);
    bool print_common_operand(mcode::Function &func, mcode::Operand &operand);

    void emit(std::string_view value);
    void emit(char c);
    void emit(int value);
    void emit(unsigned value);
    void emit(long long value);
    void emit(unsigned long long value);
    void emit(float value);
    void emit(double value);
};

} // namespace banjo::mcode

#endif
