#include "nasm_emitter.hpp"

#include "banjo/config/config.hpp"
#include "banjo/target/x86_64/x86_64_opcode.hpp"
#include "banjo/target/x86_64/x86_64_register.hpp"
#include "banjo/utils/macros.hpp"
#include "banjo/utils/timing.hpp"

#include <algorithm>
#include <sstream>
#include <unordered_map>
#include <unordered_set>
#include <variant>

namespace banjo {

namespace codegen {

const std::unordered_map<mcode::Opcode, std::string> NASMEmitter::OPCODE_NAMES = {
    {target::X8664Opcode::MOV, "mov"},
    {target::X8664Opcode::PUSH, "push"},
    {target::X8664Opcode::POP, "pop"},
    {target::X8664Opcode::ADD, "add"},
    {target::X8664Opcode::SUB, "sub"},
    {target::X8664Opcode::IMUL, "imul"},
    {target::X8664Opcode::IDIV, "idiv"},
    {target::X8664Opcode::AND, "and"},
    {target::X8664Opcode::OR, "or"},
    {target::X8664Opcode::XOR, "xor"},
    {target::X8664Opcode::SHL, "shl"},
    {target::X8664Opcode::SHR, "shr"},
    {target::X8664Opcode::CDQ, "cdq"},
    {target::X8664Opcode::CQO, "cqo"},
    {target::X8664Opcode::JMP, "jmp"},
    {target::X8664Opcode::CMP, "cmp"},
    {target::X8664Opcode::JE, "je"},
    {target::X8664Opcode::JNE, "jne"},
    {target::X8664Opcode::JA, "ja"},
    {target::X8664Opcode::JAE, "jae"},
    {target::X8664Opcode::JB, "jb"},
    {target::X8664Opcode::JBE, "jbe"},
    {target::X8664Opcode::JG, "jg"},
    {target::X8664Opcode::JGE, "jge"},
    {target::X8664Opcode::JL, "jl"},
    {target::X8664Opcode::JLE, "jle"},
    {target::X8664Opcode::CMOVE, "cmove"},
    {target::X8664Opcode::CMOVNE, "cmovne"},
    {target::X8664Opcode::CMOVA, "cmova"},
    {target::X8664Opcode::CMOVAE, "cmovae"},
    {target::X8664Opcode::CMOVB, "cmovb"},
    {target::X8664Opcode::CMOVBE, "cmovbe"},
    {target::X8664Opcode::CMOVG, "cmovg"},
    {target::X8664Opcode::CMOVGE, "cmovge"},
    {target::X8664Opcode::CMOVL, "cmovl"},
    {target::X8664Opcode::CMOVLE, "cmovle"},
    {target::X8664Opcode::CALL, "call"},
    {target::X8664Opcode::RET, "ret"},
    {target::X8664Opcode::LEA, "lea"},
    {target::X8664Opcode::MOVSX, "movsx"},
    {target::X8664Opcode::MOVZX, "movzx"},
    {target::X8664Opcode::MOVSS, "movss"},
    {target::X8664Opcode::MOVSD, "movsd"},
    {target::X8664Opcode::MOVAPS, "movaps"},
    {target::X8664Opcode::MOVUPS, "movups"},
    {target::X8664Opcode::MOVD, "movd"},
    {target::X8664Opcode::MOVQ, "movq"},
    {target::X8664Opcode::ADDSS, "addss"},
    {target::X8664Opcode::ADDSD, "addsd"},
    {target::X8664Opcode::SUBSS, "subss"},
    {target::X8664Opcode::SUBSD, "subsd"},
    {target::X8664Opcode::MULSS, "mulss"},
    {target::X8664Opcode::MULSD, "mulsd"},
    {target::X8664Opcode::DIVSS, "divss"},
    {target::X8664Opcode::DIVSD, "divsd"},
    {target::X8664Opcode::XORPS, "xorps"},
    {target::X8664Opcode::XORPD, "xorpd"},
    {target::X8664Opcode::MINSS, "minss"},
    {target::X8664Opcode::MINSD, "minsd"},
    {target::X8664Opcode::MAXSS, "maxss"},
    {target::X8664Opcode::MAXSD, "maxsd"},
    {target::X8664Opcode::SQRTSS, "sqrtss"},
    {target::X8664Opcode::SQRTSD, "sqrtsd"},
    {target::X8664Opcode::UCOMISS, "ucomiss"},
    {target::X8664Opcode::UCOMISD, "ucomisd"},
    {target::X8664Opcode::CVTSS2SD, "cvtss2sd"},
    {target::X8664Opcode::CVTSD2SS, "cvtsd2ss"},
    {target::X8664Opcode::CVTSI2SS, "cvtsi2ss"},
    {target::X8664Opcode::CVTSS2SI, "cvtss2si"},
    {target::X8664Opcode::CVTSI2SD, "cvtsi2sd"},
    {target::X8664Opcode::CVTSD2SI, "cvtsd2si"},

    {mcode::PseudoOpcode::EH_PUSHREG, ".eh_pushreg"},
    {mcode::PseudoOpcode::EH_ALLOCSTACK, ".eh_allocstack"},
};

NASMEmitter::NASMEmitter(mcode::Module &module, std::ostream &stream, target::TargetDescription target)
  : Emitter(module, stream),
    target(target) {}

void NASMEmitter::generate() {
    PROFILE_SCOPE("nasm code emitter");

    stream << "default rel\n\n";

    for (const std::string &external_symbol : module.get_external_symbols()) {
        stream << "extern ";

        if (target.get_operating_system() == target::OperatingSystem::MACOS) {
            stream << "_";
            symbol_prefixes[external_symbol] = "_";
        }

        stream << external_symbol << "\n";
    }
    stream << "\n";

    for (const std::string &global_symbol : module.get_global_symbols()) {
        stream << "global ";

        if (target.get_operating_system() == target::OperatingSystem::MACOS) {
            stream << "_";
        }

        stream << global_symbol << "\n";
    }
    stream << "\n";

    if (target.get_operating_system() == target::OperatingSystem::MACOS) {
        for (mcode::Function *function : module.get_functions()) {
            symbol_prefixes.insert({function->get_name(), "_"});
        }
    }

    stream << "section .text\n";

    for (mcode::Function *function : module.get_functions()) {
        emit_func(function);
    }

    stream << "\n";
    stream << "section .data\n";

    for (mcode::Global &global : module.get_globals()) {
        stream << global.name << " ";

        if (auto value = std::get_if<mcode::Global::Integer>(&global.value)) {
            stream << get_size_declaration(global.size) << " " << value->to_string();
        } else if (auto value = std::get_if<mcode::Global::FloatingPoint>(&global.value)) {
            std::string string = std::to_string(*value);
            if (string.find('.') == std::string::npos) {
                string += ".0";
            }

            switch (global.size) {
                case 4: stream << "dd __float32__(" << string << ")"; break;
                case 8: stream << "dq __float64__(" << string << ")"; break;
                default: ASSERT_UNREACHABLE;
            }
        } else if (auto value = std::get_if<mcode::Global::Bytes>(&global.value)) {
            stream << "db ";

            for (unsigned i = 0; i < value->size(); i++) {
                stream << (unsigned)(*value)[i];

                if (i != value->size() - 1) {
                    stream << ", ";
                }
            }
        } else if (auto value = std::get_if<mcode::Global::String>(&global.value)) {
            std::string str = "\'";

            for (char c : *value) {
                if (c == '\0') str += "\', 0x00, \'";
                else if (c == '\n') str += "\', 0x0A, \'";
                else if (c == '\r') str += "\', 0x0D, \'";
                else str += c;
            }

            str += "\'";

            size_t start_pos;
            while ((start_pos = str.find(", \'\'")) != std::string::npos) {
                str.erase(start_pos, 4);
            }

            stream << "db " << str;
        } else if (auto value = std::get_if<mcode::Global::SymbolRef>(&global.value)) {
            stream << get_size_declaration(global.size) << " " << value->name;
        } else if (std::holds_alternative<mcode::Global::None>(global.value)) {
            stream << "times " << global.size << " db 0";
        } else {
            ASSERT_UNREACHABLE;
        }

        stream << "\n";
    }

    if (!module.get_dll_exports().empty()) {
        stream << "\nsection .drectve info\n";

        stream << "db '";
        for (const std::string &dll_export : module.get_dll_exports()) {
            stream << "/EXPORT:" << dll_export << " ";
        }
        stream << "'";

        stream << "\n";
    }
}

void NASMEmitter::emit_func(mcode::Function *func) {
    if (target.get_operating_system() == target::OperatingSystem::MACOS) {
        stream << "_";
    }

    stream << func->get_name() << ":\n";

    for (mcode::BasicBlock &basic_block : func->get_basic_blocks()) {
        gen_basic_block(basic_block);
    }

    stream << "\n";
}

void NASMEmitter::gen_basic_block(mcode::BasicBlock &basic_block) {
    if (!basic_block.get_label().empty()) {
        stream << basic_block.get_label() << ":\n";
    }

    for (mcode::Instruction &instr : basic_block) {
        if (instr.get_opcode() < 0) {
            continue;
        }

        stream << "    ";
        emit_instr(basic_block, instr);
        stream << "\n";
    }
}

void NASMEmitter::emit_instr(mcode::BasicBlock &basic_block, mcode::Instruction &instr) {
    std::string line = OPCODE_NAMES.find(instr.get_opcode())->second;

    bool has_reg_operand = false;
    for (int i = 0; i < instr.get_operands().get_size(); i++) {
        if (instr.get_operand(i).is_physical_reg()) {
            has_reg_operand = true;
            break;
        }
    }

    if (!has_reg_operand && instr.has_dest() && instr.get_dest().get_size() != 0) {
        line += " " + get_size_specifier(instr.get_dest().get_size());
    }

    bool requires_size =
        instr.get_opcode() == target::X8664Opcode::MOVSX || instr.get_opcode() == target::X8664Opcode::MOVZX ||
        instr.get_opcode() == target::X8664Opcode::SHL || instr.get_opcode() == target::X8664Opcode::SHR ||
        instr.get_opcode() == target::X8664Opcode::CVTSI2SS || instr.get_opcode() == target::X8664Opcode::CVTSI2SD;

    for (int j = 0; j < instr.get_operands().get_size(); j++) {
        line += (j == 0 ? " " : ", ");

        mcode::Operand &operand = instr.get_operand(j);

        if (requires_size && !operand.is_register()) {
            line += get_size_specifier(operand.get_size()) + " ";
        }

        line += get_operand_name(basic_block, operand);
    }

    stream << line;
}

std::string NASMEmitter::get_operand_name(mcode::BasicBlock &basic_block, mcode::Operand operand) {
    if (operand.is_immediate()) return operand.get_immediate();
    else if (operand.is_register()) return get_reg_name(basic_block, operand.get_register(), operand.get_size());
    else if (operand.is_symbol()) return gen_symbol(operand.get_symbol());
    else if (operand.is_label()) return operand.get_label();
    else if (operand.is_symbol_deref()) return "[" + gen_symbol(operand.get_deref_symbol()) + "]";
    else if (operand.is_addr()) {
        mcode::IndirectAddress addr = operand.get_addr();

        std::string base;
        if (addr.get_base().is_stack_slot()) {
            base = get_stack_slot_name(basic_block.get_func(), addr.get_base(), false);
        } else {
            base = get_reg_name(basic_block, addr.get_base(), 8);
        }

        if (addr.has_offset()) {
            std::string offset;
            if (addr.has_reg_offset()) {
                if (addr.get_reg_offset().is_stack_slot()) {
                    offset = get_stack_slot_name(basic_block.get_func(), addr.get_reg_offset(), false);
                } else {
                    offset = get_reg_name(basic_block, addr.get_reg_offset(), 8);
                }
            } else if (addr.has_int_offset()) offset = std::to_string(addr.get_int_offset());

            std::string scaled_offset =
                addr.get_scale() == 1 ? offset : std::to_string(addr.get_scale()) + " * " + offset;
            return "[" + base + " + " + scaled_offset + "]";
        } else {
            return "[" + base + "]";
        }
    } else return "???";
}

std::string NASMEmitter::get_reg_name(mcode::BasicBlock &basic_block, mcode::Register reg, int size) {
    if (reg.is_virtual_reg()) return "%" + std::to_string(reg.get_virtual_reg());
    else if (reg.is_physical_reg()) return get_physical_reg_name(reg.get_physical_reg(), size);
    else if (reg.is_stack_slot()) return get_stack_slot_name(basic_block.get_func(), reg);
    else return "???";
}

std::string NASMEmitter::get_physical_reg_name(long reg, int size) {
    if (size == 0) {
        size = 4;
    }

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

    return "???";
}

std::string NASMEmitter::get_stack_slot_name(mcode::Function *func, mcode::Register reg, bool brackets /* = true */) {
    mcode::StackFrame &frame = func->get_stack_frame();
    mcode::StackSlot &slot = frame.get_stack_slot(reg.get_stack_slot());
    int offset = slot.get_offset();
    std::string offset_str = offset >= 0 ? "+ " + std::to_string(offset) : "- " + std::to_string(-offset);

    std::string address = "rsp " + offset_str;
    return brackets ? "[" + address + "]" : address;
}

std::string NASMEmitter::gen_symbol(const mcode::Symbol &symbol) {
    std::string string = symbol_prefixes[symbol.name] + symbol.name;

    if (symbol.reloc == mcode::Relocation::GOT) string += " wrt ..got";
    else if (symbol.reloc == mcode::Relocation::PLT) string += " wrt ..plt";

    return string;
}

std::string NASMEmitter::get_size_specifier(int size) {
    switch (size) {
        case 1: return "byte";
        case 2: return "word";
        case 4: return "dword";
        case 8: return "qword";
        case 16: return "oword";
        default: return "???";
    }
}

std::string NASMEmitter::get_size_declaration(int size) {
    switch (size) {
        case 1: return "db";
        case 2: return "dw";
        case 4: return "dd";
        case 8: return "dq";
        default: ASSERT_UNREACHABLE;
    }
}

} // namespace codegen

} // namespace banjo
