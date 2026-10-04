#include "printer.hpp"

#include "banjo/mcode/function.hpp"
#include "banjo/mcode/instruction.hpp"
#include "banjo/mcode/module.hpp"
#include <string>

namespace banjo::mcode {

Printer::Printer(std::string &buffer) : buffer{buffer} {}

void Printer::print(mcode::Module &mod) {
    bool first = true;

    for (mcode::Function *func : mod.get_functions()) {
        if (first) {
            first = false;
            emit('\n');
        }

        print_func(*func);
    }
}

void Printer::print_func(mcode::Function &func) {
    emit("func ");
    emit(func.name);
    emit(":\n");

    for (mcode::BasicBlock &block : func.basic_blocks) {
        if (!block.label.empty()) {
            emit(block.label);
            emit(":\n");
        }

        for (mcode::Instruction &instr : block.instrs) {
            emit("    ");
            print_opcode(instr);

            for (unsigned i = 0; i < instr.get_operands().size(); i++) {
                emit(i == 0 ? " " : ", ");
                print_operand(instr, i);
            }

            emit('\n');
        }
    }
}

void Printer::print_virtual_reg(mcode::VirtualReg reg, unsigned size) {
    emit(size);
    emit("b %");
    emit(reg);
}

void Printer::emit(std::string_view value) {
    buffer += value;
}

void Printer::emit(char c) {
    buffer += c;
}

void Printer::emit(int value) {
    buffer += std::to_string(value);
}

void Printer::emit(unsigned value) {
    buffer += std::to_string(value);
}

void Printer::emit(long long value) {
    buffer += std::to_string(value);
}

void Printer::emit(unsigned long long value) {
    buffer += std::to_string(value);
}

void Printer::emit(float value) {
    buffer += std::to_string(value);
}

void Printer::emit(double value) {
    buffer += std::to_string(value);
}

} // namespace banjo::mcode
