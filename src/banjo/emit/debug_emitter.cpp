#include "debug_emitter.hpp"

#include "banjo/config/config.hpp"
#include "banjo/emit/aarch64_asm_emitter.hpp"
#include "banjo/emit/nasm_emitter.hpp"
#include "banjo/target/aarch64/aarch64_register.hpp"
#include "banjo/target/target_description.hpp"
#include "banjo/target/wasm/wasm_opcode.hpp"
#include "banjo/target/x86_64/x86_64_register.hpp"
#include "banjo/utils/macros.hpp"
#include "banjo/utils/timing.hpp"

#include <cassert>
#include <unordered_map>
#include <unordered_set>

namespace banjo {

namespace codegen {

DebugEmitter::DebugEmitter(mcode::Module &module, std::ostream &stream, target::TargetDescription target)
  : Emitter(module, stream),
    target(target) {}

void DebugEmitter::generate() {
    PROFILE_SCOPE("debug code emitter");

    for (const std::string &external_symbol : module.get_external_symbols()) {
        stream << "extern " << external_symbol << "\n";
    }
    stream << "\n";

    for (const std::string &global_symbol : module.get_global_symbols()) {
        stream << "global " << global_symbol << "\n";
    }
    stream << "\n";

    for (mcode::Function *function : module.get_functions()) {
        generate(function);
    }
    stream << "\n";

    for (mcode::Global &global : module.get_globals()) {
        stream << "data " << global.name << ": " << global.size << "b = ";

        if (auto value = std::get_if<mcode::Global::Integer>(&global.value)) {
            stream << value->to_string();
        } else if (auto value = std::get_if<mcode::Global::FloatingPoint>(&global.value)) {
            stream << *value;
        } else if (auto value = std::get_if<mcode::Global::Bytes>(&global.value)) {
            stream << "[";

            for (unsigned i = 0; i < value->size(); i++) {
                stream << (unsigned)(*value)[i];

                if (i != value->size() - 1) {
                    stream << ", ";
                }
            }

            stream << "]";
        } else if (auto value = std::get_if<mcode::Global::String>(&global.value)) {
            stream << "\"";

            for (char c : *value) {
                if (c == '\0') stream << "\\0";
                else if (c == '\n') stream << "\\n";
                else if (c == '\r') stream << "\\r";
                else stream << c;
            }

            stream << "\"";
        } else if (auto value = std::get_if<mcode::Global::SymbolRef>(&global.value)) {
            stream << "@" << value->name;
        } else if (std::holds_alternative<mcode::Global::None>(global.value)) {
            stream << "undefined";
        } else {
            ASSERT_UNREACHABLE;
        }

        stream << "\n";
    }
}

void DebugEmitter::generate(mcode::Function *func) {
    stream << "func " << func->get_name() << ":\n";

    mcode::StackFrame &frame = func->get_stack_frame();
    if (!frame.get_stack_slots().empty()) {
        for (unsigned i = 0; i < frame.get_stack_slots().size(); i++) {
            mcode::StackSlot &slot = frame.get_stack_slot(i);

            stream << "    s" << i << ": ";

            if (!slot.is_defined()) {
                stream << "?";
            } else {
                if (slot.get_offset() >= 0) {
                    stream << "+";
                }
                stream << slot.get_offset();
            }
            stream << ", ";

            stream << slot.get_size() << ", ";

            switch (slot.get_type()) {
                case mcode::StackSlot::Type::GENERIC: stream << "generic"; break;
                case mcode::StackSlot::Type::ARG_STORE: stream << "arg_store"; break;
                case mcode::StackSlot::Type::CALL_ARG: stream << "call_arg"; break;
            }
            stream << "\n";
        }
        stream << "\n";
    }

    for (mcode::BasicBlock &basic_block : func->get_basic_blocks()) {
        gen_basic_block(basic_block);
    }

    stream << "\n";
}

void DebugEmitter::gen_basic_block(mcode::BasicBlock &basic_block) {
    if (!basic_block.get_label().empty()) {
        stream << basic_block.get_label();

        if (!basic_block.get_params().empty()) {
            stream << "(";

            for (unsigned i = 0; i < basic_block.get_params().size(); i++) {
                stream << "%" << basic_block.get_params()[i];
                if (i != basic_block.get_params().size() - 1) {
                    stream << ", ";
                }
            }

            stream << ")";
        }

        stream << ":\n";
    }

    /*
    stream << "  preds: ";
    for (unsigned i = 0; i < basic_block.get_predecessors().size(); i++) {
        stream << basic_block.get_predecessors()[i]->get_label();
        if (i != basic_block.get_predecessors().size() - 1) {
            stream << ", ";
        }
    }
    stream << "\n  succs: ";
    for (unsigned i = 0; i < basic_block.get_successors().size(); i++) {
        stream << basic_block.get_successors()[i]->get_label();
        if (i != basic_block.get_successors().size() - 1) {
            stream << ", ";
        }
    }
    stream << "\n";
    */

    std::unordered_set<mcode::MachineInstrNode *> nodes;
    for (mcode::InstrIter iter = basic_block.begin(); iter != basic_block.end(); ++iter) {
        assert(!nodes.contains(iter.get_node()));
        nodes.insert(iter.get_node());
    }

    for (mcode::Instruction &instr : basic_block) {
        stream << instr_to_string(basic_block, instr) << "\n";
    }
}

std::string DebugEmitter::instr_to_string(mcode::BasicBlock &basic_block, mcode::Instruction &instr) {
    std::string string = "  ";

    string += get_opcode_name(instr.get_opcode());

    for (unsigned int i = 0; i < instr.get_operands().get_size(); i++) {
        mcode::Operand &operand = instr.get_operands()[i];

        string += (i == 0 ? " " : ", ");

        if (operand.get_size() != 0 && !operand.is_physical_reg()) {
            string += std::to_string(operand.get_size()) + "b ";
        }

        string += get_operand_name(basic_block, operand, i);
    }

    if (instr.get_flags() & mcode::Instruction::FLAG_ARG_STORE) string += " !arg_store";
    if (instr.get_flags() & mcode::Instruction::FLAG_ALLOCA) string += " !alloca";
    if (instr.get_flags() & mcode::Instruction::FLAG_CALL_ARG) string += " !call_arg";
    if (instr.get_flags() & mcode::Instruction::FLAG_CALL) string += " !call";
    if (instr.get_flags() & mcode::Instruction::FLAG_FLOAT) string += " !float";

    return string;
}

std::string_view DebugEmitter::get_opcode_name(mcode::Opcode opcode) {
    target::Architecture arch = lang::Config::instance().target.get_architecture();

    if (arch == target::Architecture::X86_64) {
        return NASMEmitter::OPCODE_NAMES.at(opcode);
    } else if (arch == target::Architecture::AARCH64) {
        return AArch64AsmEmitter::OPCODE_NAMES.at(opcode);
    } else if (arch == target::Architecture::WASM) {
        return target::WasmOpcode::NAMES.at(opcode);
    }

    return "";
}

std::string DebugEmitter::get_operand_name(mcode::BasicBlock &basic_block, mcode::Operand operand, int instr_index) {
    if (operand.is_int_immediate()) return operand.get_int_immediate().to_string();
    else if (operand.is_fp_immediate()) return std::to_string(operand.get_fp_immediate());
    else if (operand.is_register())
        return get_reg_name(basic_block, operand.get_register(), operand.get_size(), instr_index);
    else if (operand.is_stack_slot())
        return get_stack_slot_name(basic_block.get_func(), operand.get_stack_slot(), instr_index);
    else if (operand.is_symbol()) return operand.get_symbol().name;
    else if (operand.is_label()) return operand.get_label();
    else if (operand.is_symbol_deref()) return "[" + operand.get_deref_symbol().name + "]";
    else if (operand.is_addr()) {
        mcode::IndirectAddress addr = operand.get_addr();

        std::string base;
        if (addr.is_base_reg()) {
            base = get_reg_name(basic_block, addr.get_base_reg(), 8, instr_index);
        } else if (addr.is_base_stack_slot()) {
            base = get_stack_slot_name(basic_block.get_func(), addr.get_base_stack_slot(), instr_index, false);
        } else {
            ASSERT_UNREACHABLE;
        }

        if (addr.has_offset()) {
            std::string offset;
            if (addr.has_reg_offset()) offset = get_reg_name(basic_block, addr.get_reg_offset(), 8, instr_index);
            else if (addr.has_int_offset()) offset = std::to_string(addr.get_int_offset());

            std::string scaled_offset =
                addr.get_scale() == 1 ? offset : std::to_string(addr.get_scale()) + " * " + offset;
            return "[" + base + " + " + scaled_offset + "]";
        } else {
            return "[" + base + "]";
        }
    } else if (operand.is_aarch64_addr()) {
        target::AArch64Address addr = operand.get_aarch64_addr();
        std::string str = "[";

        switch (addr.get_type()) {
            case target::AArch64Address::Type::BASE:
                str += get_reg_name(basic_block, addr.get_base(), 8, instr_index) + "]";
                break;
            case target::AArch64Address::Type::BASE_OFFSET_IMM:
                str += get_reg_name(basic_block, addr.get_base(), 8, instr_index);
                str += ", #" + std::to_string(addr.get_offset_imm()) + "]";
                break;
            case target::AArch64Address::Type::BASE_OFFSET_IMM_WRITE:
                str += get_reg_name(basic_block, addr.get_base(), 8, instr_index);
                str += ", #" + std::to_string(addr.get_offset_imm()) + "]!";
                break;
            case target::AArch64Address::Type::BASE_OFFSET_REG:
                str += get_reg_name(basic_block, addr.get_base(), 8, instr_index);
                str += ", ";
                str += get_reg_name(basic_block, addr.get_offset_reg(), 8, instr_index);
                str += "]";
                break;
            case target::AArch64Address::Type::BASE_OFFSET_SYMBOL:
                str += get_reg_name(basic_block, addr.get_base(), 8, instr_index);
                str += ", ";
                str += addr.get_offset_symbol().name;
                str += "]";
                break;
        }

        return str;
    } else if (operand.is_aarch64_left_shift()) {
        return "lsl #" + std::to_string(operand.get_aarch64_left_shift());
    } else {
        ASSERT_UNREACHABLE;
    }
}

std::string DebugEmitter::get_reg_name(mcode::BasicBlock &basic_block, mcode::Register reg, int size, int instr_index) {
    if (reg.is_virtual()) return "%" + std::to_string(reg.get_virtual_reg());
    else if (reg.is_physical()) return get_physical_reg_name(reg.get_physical_reg(), size);
    else ASSERT_UNREACHABLE;
}

std::string DebugEmitter::get_physical_reg_name(long reg, int size) {
    if (size == 0) {
        size = 4;
    }

    if (lang::Config::instance().target.get_architecture() == target::Architecture::X86_64) {
        if (reg >= target::X8664Register::RAX && reg <= target::X8664Register::RBX) {
            char letter;
            switch (reg) {
                case target::X8664Register::RAX: letter = 'a'; break;
                case target::X8664Register::RDX: letter = 'd'; break;
                case target::X8664Register::RCX: letter = 'c'; break;
                case target::X8664Register::RBX: letter = 'b'; break;
            }

            switch (size) {
                case 1: return std::string(1, letter) + "l";
                case 2: return std::string(1, letter) + "x";
                case 4: return "e" + std::string(1, letter) + "x";
                case 8: return "r" + std::string(1, letter) + "x";
            }
        } else if (reg == target::X8664Register::RSP || reg == target::X8664Register::RBP) {
            char letter;
            switch (reg) {
                case target::X8664Register::RSP: letter = 's'; break;
                case target::X8664Register::RBP: letter = 'b'; break;
            }

            switch (size) {
                case 1: return std::string(1, letter) + "pl";
                case 2: return std::string(1, letter) + "p";
                case 4: return "e" + std::string(1, letter) + "p";
                case 8: return "r" + std::string(1, letter) + "p";
            }
        } else if (reg == target::X8664Register::RSI || reg == target::X8664Register::RDI) {
            char letter;
            switch (reg) {
                case target::X8664Register::RSI: letter = 's'; break;
                case target::X8664Register::RDI: letter = 'd'; break;
            }

            switch (size) {
                case 1: return std::string(1, letter) + "il";
                case 2: return std::string(1, letter) + "i";
                case 4: return "e" + std::string(1, letter) + "i";
                case 8: return "r" + std::string(1, letter) + "i";
            }
        } else if (reg >= target::X8664Register::R8 && reg <= target::X8664Register::R15) {
            std::string number;
            switch (reg) {
                case target::X8664Register::R8: number = "8"; break;
                case target::X8664Register::R9: number = "9"; break;
                case target::X8664Register::R10: number = "10"; break;
                case target::X8664Register::R11: number = "11"; break;
                case target::X8664Register::R12: number = "12"; break;
                case target::X8664Register::R13: number = "13"; break;
                case target::X8664Register::R14: number = "14"; break;
                case target::X8664Register::R15: number = "15"; break;
            }

            switch (size) {
                case 1: return "r" + number + "b";
                case 2: return "r" + number + "w";
                case 4: return "r" + number + "d";
                case 8: return "r" + number;
            }
        } else if (reg >= target::X8664Register::XMM0 && reg <= target::X8664Register::XMM15) {
            switch (reg) {
                case target::X8664Register::XMM0: return "xmm0";
                case target::X8664Register::XMM1: return "xmm1";
                case target::X8664Register::XMM2: return "xmm2";
                case target::X8664Register::XMM3: return "xmm3";
                case target::X8664Register::XMM4: return "xmm4";
                case target::X8664Register::XMM5: return "xmm5";
                case target::X8664Register::XMM6: return "xmm6";
                case target::X8664Register::XMM7: return "xmm7";
                case target::X8664Register::XMM8: return "xmm8";
                case target::X8664Register::XMM9: return "xmm9";
                case target::X8664Register::XMM10: return "xmm10";
                case target::X8664Register::XMM11: return "xmm11";
                case target::X8664Register::XMM12: return "xmm12";
                case target::X8664Register::XMM13: return "xmm13";
                case target::X8664Register::XMM14: return "xmm14";
                case target::X8664Register::XMM15: return "xmm15";
            }
        }
    } else if (lang::Config::instance().target.get_architecture() == target::Architecture::AARCH64) {
        if (reg >= target::AArch64Register::R0 && reg <= target::AArch64Register::R_LAST) {
            return (size == 8 ? "x" : "w") + std::to_string(reg - target::AArch64Register::R0);
        } else if (reg >= target::AArch64Register::V0 && reg <= target::AArch64Register::V_LAST) {
            std::string c;

            switch (size) {
                case 1: c = "b"; break;
                case 2: c = "h"; break;
                case 4: c = "s"; break;
                case 8: c = "d"; break;
                case 16: c = "q"; break;
                default: c = "?"; break;
            }

            return c + std::to_string(reg - target::AArch64Register::V0);
        } else if (reg == target::AArch64Register::SP) {
            return "sp";
        }
    }

    return "???";
}

std::string DebugEmitter::get_stack_slot_name(
    mcode::Function *func,
    mcode::StackSlotID stack_slot,
    int instr_index,
    bool brackets /* = true */
) {
    mcode::StackFrame &frame = func->get_stack_frame();
    mcode::StackSlot &slot = frame.get_stack_slot(stack_slot);
    int offset = slot.get_offset();

    if (offset == mcode::StackSlot::INVALID_OFFSET) {
        return "s" + std::to_string(stack_slot);
    }

    if (lang::Config::instance().target.get_architecture() == target::Architecture::X86_64) {
        std::string offset_str = offset >= 0 ? "+ " + std::to_string(offset) : "- " + std::to_string(-offset);
        std::string address = "rsp " + offset_str;
        return brackets ? "[" + address + "]" : address;
    } else if (lang::Config::instance().target.get_architecture() == target::Architecture::AARCH64) {
        return +"[sp, #" + std::to_string(slot.get_offset()) + "]";
    }

    return "";
}

std::string DebugEmitter::get_size_specifier(int size) {
    return "i" + std::to_string(8 * size);
}

} // namespace codegen

} // namespace banjo
