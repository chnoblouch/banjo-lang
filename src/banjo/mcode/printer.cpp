#include "printer.hpp"

#include "banjo/mcode/function.hpp"
#include "banjo/mcode/instruction.hpp"
#include "banjo/mcode/module.hpp"
#include "banjo/utils/macros.hpp"

#include <string>

namespace banjo::mcode {

Printer &Printer::set_buffer(std::string &buffer) {
    this->buffer = &buffer;
    return *this;
}

Printer &Printer::set_flags(unsigned flags) {
    this->flags |= flags;
    return *this;
}

void Printer::print(mcode::Module &mod) {
    for (std::string_view external : mod.external_symbols) {
        print_external(external);
        emit('\n');
    }

    if (!mod.external_symbols.empty()) {
        emit('\n');
    }

    for (std::string_view global : mod.global_symbols) {
        print_global(global);
        emit('\n');
    }

    if (!mod.global_symbols.empty()) {
        emit('\n');
    }

    for (mcode::Global &data : mod.globals) {
        print_data(data);
        emit('\n');
    }

    if (!mod.global_symbols.empty()) {
        emit('\n');
    }

    bool first = mod.external_symbols.empty() && mod.global_symbols.empty() && mod.globals.empty();

    for (mcode::Function *func : mod.functions) {
        if (first) {
            first = false;
        } else {
            emit('\n');
        }

        print_func(*func);
    }
}

void Printer::print_external(std::string_view name) {
    emit("extern ");
    emit(name);
}

void Printer::print_global(std::string_view name) {
    emit("global ");
    emit(name);
}

void Printer::print_data(mcode::Global &global) {
    emit("data ");
    emit(global.name);
    emit(": ");

    if (auto value = std::get_if<mcode::Global::Integer>(&global.value)) {
        emit(value->to_string());
    } else if (auto value = std::get_if<mcode::Global::FloatingPoint>(&global.value)) {
        emit(*value);
    } else if (auto value = std::get_if<mcode::Global::Bytes>(&global.value)) {
        emit('[');

        for (unsigned i = 0; i < value->size(); i++) {
            emit(static_cast<unsigned>((*value)[i]));

            if (i != value->size() - 1) {
                emit(", ");
            }
        }

        emit(']');
    } else if (auto value = std::get_if<mcode::Global::String>(&global.value)) {
        emit('\"');

        for (char c : *value) {
            // TODO: Incomplete
            switch (c) {
                case '\0': emit("\\0"); break;
                case '\n': emit("\\n"); break;
                case '\r': emit("\\r"); break;
                default: emit(c); break;
            }
        }

        emit('\"');
    } else if (auto value = std::get_if<mcode::Global::SymbolRef>(&global.value)) {
        emit(value->name); // TODO: Relocations
    } else if (std::holds_alternative<mcode::Global::None>(global.value)) {
        emit("undefined");
    } else {
        ASSERT_UNREACHABLE;
    }
}

void Printer::print_func(mcode::Function &func) {
    emit("func ");
    emit(func.name);
    emit(":\n");

    for (unsigned i = 0; i < func.stack_frame.get_stack_slots().size(); i++) {
        emit("    ");
        print_stack_slot(i, func.stack_frame.get_stack_slots()[i]);
        emit('\n');
    }

    if (!func.stack_frame.get_stack_slots().empty()) {
        emit('\n');
    }

    for (mcode::BasicBlock &block : func.basic_blocks) {
        if (!block.label.empty()) {
            emit(block.label);
            emit(":\n");
        }

        for (mcode::Instruction &instr : block.instrs) {
            print_instr(func, instr);
            emit('\n');
        }
    }
}

void Printer::print_stack_slot(mcode::StackSlotID id, mcode::StackSlot &slot) {
    emit('s');
    emit(id);
    emit(": off ");

    if (slot.is_defined()) {
        emit(slot.offset);
    } else {
        emit("undefined");
    }

    emit(", size ");
    emit(slot.size);
    emit(", ");

    switch (slot.type) {
        case mcode::StackSlot::Type::GENERIC: emit("generic"); break;
        case mcode::StackSlot::Type::ARG_STORE: emit("arg_store"); break;
        case mcode::StackSlot::Type::CALL_ARG: emit("call_arg"); break;
    }
}

void Printer::print_instr(mcode::Function &func, mcode::Instruction &instr) {
    emit("    ");

    switch (instr.get_opcode()) {
        case PseudoOpcode::EH_PUSHREG: emit(".eh_pushreg"); break;
        case PseudoOpcode::EH_ALLOCSTACK: emit(".eh_allocstack"); break;
        case PseudoOpcode::EH_ENDPROLOG: emit(".eh_endprolog"); break;
        default: print_opcode(instr); break;
    }

    for (unsigned i = 0; i < instr.get_operands().size(); i++) {
        emit(i == 0 ? " " : ", ");
        print_operand(func, instr, i);
    }

    if (flags & NO_ATTRIBUTES) {
        return;
    }

    if (instr.get_flags() & mcode::Instruction::FLAG_ARG_STORE) emit(" !arg_store");
    if (instr.get_flags() & mcode::Instruction::FLAG_ALLOCA) emit(" !alloca");
    if (instr.get_flags() & mcode::Instruction::FLAG_CALL_ARG) emit(" !call_arg");
    if (instr.get_flags() & mcode::Instruction::FLAG_CALL) emit(" !call");
    if (instr.get_flags() & mcode::Instruction::FLAG_FLOAT) emit(" !float");
}

void Printer::print_register(mcode::Register reg, unsigned size) {
    if (reg.is_virtual()) {
        print_virtual_reg(reg.get_virtual_reg(), size);
    } else if (reg.is_physical()) {
        print_physical_reg(reg.get_physical_reg(), size);
    } else {
        ASSERT_UNREACHABLE;
    }
}

void Printer::print_virtual_reg(mcode::VirtualReg reg, unsigned size) {
    emit(size);
    emit("b %");
    emit(reg);
}

void Printer::print_stack_slot(mcode::StackSlotID slot) {
    emit("s");
    emit(slot);
}

void Printer::print_stack_addr(mcode::Function &func, const mcode::StackAddress &stack_addr) {
    mcode::StackSlot &slot = func.stack_frame.get_stack_slot(stack_addr.slot);

    if (slot.is_defined()) {
        emit(func.stack_frame.offset_of(stack_addr));
    } else {
        emit("off(");
        print_stack_slot(stack_addr.slot);
        emit(")");

        if (stack_addr.offset != 0) {
            emit(" + ");
            emit(stack_addr.offset);
        }
    }
}

void Printer::print_symbol(const mcode::Symbol &symbol) {
    // TODO: Relocations
    emit(symbol.name);
}

bool Printer::print_common_operand(mcode::Function &func, mcode::Operand &operand) {
    if (operand.is_int_immediate()) {
        emit(operand.get_int_immediate().to_string());
        return true;
    } else if (operand.is_fp_immediate()) {
        emit(operand.get_fp_immediate());
        return true;
    } else if (operand.is_register()) {
        print_register(operand.get_register(), operand.get_size());
        return true;
    } else if (operand.is_stack_slot()) {
        print_stack_slot(operand.get_stack_slot());
        return true;
    } else if (operand.is_symbol()) {
        print_symbol(operand.get_symbol());
        return true;
    } else if (operand.is_basic_block()) {
        emit(operand.get_basic_block().label);
        return true;
    } else if (operand.is_stack_offset()) {
        print_stack_addr(func, operand.get_stack_offset());
        return true;
    } else {
        return false;
    }
}

void Printer::emit(std::string_view value) {
    *buffer += value;
}

void Printer::emit(char c) {
    *buffer += c;
}

void Printer::emit(int value) {
    *buffer += std::to_string(value);
}

void Printer::emit(unsigned value) {
    *buffer += std::to_string(value);
}

void Printer::emit(long long value) {
    *buffer += std::to_string(value);
}

void Printer::emit(unsigned long long value) {
    *buffer += std::to_string(value);
}

void Printer::emit(float value) {
    *buffer += std::to_string(value);
}

void Printer::emit(double value) {
    *buffer += std::to_string(value);
}

} // namespace banjo::mcode
