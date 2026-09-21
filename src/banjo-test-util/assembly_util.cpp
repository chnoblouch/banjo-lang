#include "assembly_util.hpp"

#include "banjo/emit/binary_module.hpp"
#include "banjo/mcode/function.hpp"
#include "banjo/mcode/module.hpp"
#include "banjo/mcode/register.hpp"
#include "banjo/target/aarch64/aarch64_address.hpp"
#include "banjo/target/aarch64/aarch64_condition.hpp"
#include "banjo/target/aarch64/aarch64_encoder.hpp"
#include "banjo/target/aarch64/aarch64_opcode.hpp"
#include "banjo/target/aarch64/aarch64_register.hpp"
#include "banjo/target/target_description.hpp"
#include "banjo/target/x86_64/x86_64_encoder.hpp"
#include "banjo/target/x86_64/x86_64_opcode.hpp"
#include "banjo/target/x86_64/x86_64_register.hpp"
#include "banjo/utils/hash_map.hpp"
#include "banjo/utils/macros.hpp"

#include "line_based_reader.hpp"

#include <iostream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>

namespace banjo::test {

// clang-format off
const std::unordered_map<std::string_view, mcode::Opcode> X86_64_OPCODE_MAP{
    {"mov", target::X8664Opcode::MOV},
    {"push", target::X8664Opcode::PUSH},
    {"pop", target::X8664Opcode::POP},
    {"add", target::X8664Opcode::ADD},
    {"sub", target::X8664Opcode::SUB},
    {"imul", target::X8664Opcode::IMUL},
    {"div", target::X8664Opcode::DIV},
    {"idiv", target::X8664Opcode::IDIV},
    {"and", target::X8664Opcode::AND},
    {"or", target::X8664Opcode::OR},
    {"xor", target::X8664Opcode::XOR},
    {"shl", target::X8664Opcode::SHL},
    {"shr", target::X8664Opcode::SHR},
    {"sar", target::X8664Opcode::SAR},
    {"cwd", target::X8664Opcode::CWD},
    {"cdq", target::X8664Opcode::CDQ},
    {"cqo", target::X8664Opcode::CQO},
    {"xchg", target::X8664Opcode::XCHG},
    {"jmp", target::X8664Opcode::JMP},
    {"cmp", target::X8664Opcode::CMP},
    {"je", target::X8664Opcode::JE},
    {"jne", target::X8664Opcode::JNE},
    {"ja", target::X8664Opcode::JA},
    {"jae", target::X8664Opcode::JAE},
    {"jb", target::X8664Opcode::JB},
    {"jbe", target::X8664Opcode::JBE},
    {"jg", target::X8664Opcode::JG},
    {"jge", target::X8664Opcode::JGE},
    {"jl", target::X8664Opcode::JL},
    {"jle", target::X8664Opcode::JLE},
    {"cmove", target::X8664Opcode::CMOVE},
    {"cmovne", target::X8664Opcode::CMOVNE},
    {"cmova", target::X8664Opcode::CMOVA},
    {"cmovae", target::X8664Opcode::CMOVAE},
    {"cmovb", target::X8664Opcode::CMOVB},
    {"cmovbe", target::X8664Opcode::CMOVBE},
    {"cmovg", target::X8664Opcode::CMOVG},
    {"cmovge", target::X8664Opcode::CMOVGE},
    {"cmovl", target::X8664Opcode::CMOVL},
    {"cmovle", target::X8664Opcode::CMOVLE},
    {"call", target::X8664Opcode::CALL},
    {"ret", target::X8664Opcode::RET},
    {"lea", target::X8664Opcode::LEA},
    {"movsx", target::X8664Opcode::MOVSX},
    {"movzx", target::X8664Opcode::MOVZX},
    {"movss", target::X8664Opcode::MOVSS},
    {"movsd", target::X8664Opcode::MOVSD},
    {"movaps", target::X8664Opcode::MOVAPS},
    {"movups", target::X8664Opcode::MOVUPS},
    {"movd", target::X8664Opcode::MOVD},
    {"movq", target::X8664Opcode::MOVQ},
    {"addss", target::X8664Opcode::ADDSS},
    {"addsd", target::X8664Opcode::ADDSD},
    {"subss", target::X8664Opcode::SUBSS},
    {"subsd", target::X8664Opcode::SUBSD},
    {"mulss", target::X8664Opcode::MULSS},
    {"mulsd", target::X8664Opcode::MULSD},
    {"divss", target::X8664Opcode::DIVSS},
    {"divsd", target::X8664Opcode::DIVSD},
    {"xorps", target::X8664Opcode::XORPS},
    {"xorpd", target::X8664Opcode::XORPD},
    {"minss", target::X8664Opcode::MINSS},
    {"minsd", target::X8664Opcode::MINSD},
    {"maxss", target::X8664Opcode::MAXSS},
    {"maxsd", target::X8664Opcode::MAXSD},
    {"sqrtss", target::X8664Opcode::SQRTSS},
    {"sqrtsd", target::X8664Opcode::SQRTSD},
    {"ucomiss", target::X8664Opcode::UCOMISS},
    {"ucomisd", target::X8664Opcode::UCOMISD},
    {"cvtss2sd", target::X8664Opcode::CVTSS2SD},
    {"cvtsd2ss", target::X8664Opcode::CVTSD2SS},
    {"cvtsi2ss", target::X8664Opcode::CVTSI2SS},
    {"cvtsi2sd", target::X8664Opcode::CVTSI2SD},
    {"cvtss2si", target::X8664Opcode::CVTSS2SI},
    {"cvtsd2si", target::X8664Opcode::CVTSD2SI},
};
// clang-format on

// clang-format off
const std::unordered_map<std::string_view, mcode::Opcode> AARCH64_OPCODE_MAP{
    {"mov", target::AArch64Opcode::MOV},
    {"movz", target::AArch64Opcode::MOVZ},
    {"movk", target::AArch64Opcode::MOVK},
    {"ldr", target::AArch64Opcode::LDR},
    {"ldrb", target::AArch64Opcode::LDRB},
    {"ldrh", target::AArch64Opcode::LDRH},
    {"str", target::AArch64Opcode::STR},
    {"strb", target::AArch64Opcode::STRB},
    {"strh", target::AArch64Opcode::STRH},
    {"ldp", target::AArch64Opcode::LDP},
    {"stp", target::AArch64Opcode::STP},
    {"ldar", target::AArch64Opcode::LDAR},
    {"ldarb", target::AArch64Opcode::LDARB},
    {"ldarh", target::AArch64Opcode::LDARH},
    {"stlr", target::AArch64Opcode::STLR},
    {"stlrb", target::AArch64Opcode::STLRB},
    {"stlrh", target::AArch64Opcode::STLRH},
    {"add", target::AArch64Opcode::ADD},
    {"sub", target::AArch64Opcode::SUB},
    {"mul", target::AArch64Opcode::MUL},
    {"madd", target::AArch64Opcode::MADD},
    {"msub", target::AArch64Opcode::MSUB},
    {"sdiv", target::AArch64Opcode::SDIV},
    {"udiv", target::AArch64Opcode::UDIV},
    {"and", target::AArch64Opcode::AND},
    {"orr", target::AArch64Opcode::ORR},
    {"eor", target::AArch64Opcode::EOR},
    {"lsl", target::AArch64Opcode::LSL},
    {"lsr", target::AArch64Opcode::LSR},
    {"asr", target::AArch64Opcode::ASR},
    {"csel", target::AArch64Opcode::CSEL},
    {"fmov", target::AArch64Opcode::FMOV},
    {"fadd", target::AArch64Opcode::FADD},
    {"fsub", target::AArch64Opcode::FSUB},
    {"fmul", target::AArch64Opcode::FMUL},
    {"fdiv", target::AArch64Opcode::FDIV},
    {"fcvt", target::AArch64Opcode::FCVT},
    {"scvtf", target::AArch64Opcode::SCVTF},
    {"ucvtf", target::AArch64Opcode::UCVTF},
    {"fcvtzs", target::AArch64Opcode::FCVTZS},
    {"fcvtzu", target::AArch64Opcode::FCVTZU},
    {"fcsel", target::AArch64Opcode::FCSEL},
    {"cmp", target::AArch64Opcode::CMP},
    {"fcmp", target::AArch64Opcode::FCMP},
    {"b", target::AArch64Opcode::B},
    {"br", target::AArch64Opcode::BR},
    {"b.eq", target::AArch64Opcode::B_EQ},
    {"b.ne", target::AArch64Opcode::B_NE},
    {"b.hs", target::AArch64Opcode::B_HS},
    {"b.lo", target::AArch64Opcode::B_LO},
    {"b.hi", target::AArch64Opcode::B_HI},
    {"b.ls", target::AArch64Opcode::B_LS},
    {"b.ge", target::AArch64Opcode::B_GE},
    {"b.lt", target::AArch64Opcode::B_LT},
    {"b.gt", target::AArch64Opcode::B_GT},
    {"b.le", target::AArch64Opcode::B_LE},
    {"bl", target::AArch64Opcode::BL},
    {"blr", target::AArch64Opcode::BLR},
    {"ret", target::AArch64Opcode::RET},
    {"adrp", target::AArch64Opcode::ADRP},
    {"uxtb", target::AArch64Opcode::UXTB},
    {"uxth", target::AArch64Opcode::UXTH},
    {"sxtb", target::AArch64Opcode::SXTB},
    {"sxth", target::AArch64Opcode::SXTH},
    {"sxtw", target::AArch64Opcode::SXTW},
};
// clang-format on

// clang-format off
const HashMap<std::string_view, std::pair<mcode::PhysicalReg, unsigned>> X86_64_GP_REG_MAP{
    {"rax", {target::X8664Register::RAX, 8}},
    {"rcx", {target::X8664Register::RCX, 8}},
    {"rdx", {target::X8664Register::RDX, 8}},
    {"rbx", {target::X8664Register::RBX, 8}},
    {"rsi", {target::X8664Register::RSI, 8}},
    {"rdi", {target::X8664Register::RDI, 8}},
    {"rsp", {target::X8664Register::RSP, 8}},
    {"rbp", {target::X8664Register::RBP, 8}},
    {"r8", {target::X8664Register::R8, 8}},
    {"r9", {target::X8664Register::R9, 8}},
    {"r10", {target::X8664Register::R10, 8}},
    {"r11", {target::X8664Register::R11, 8}},
    {"r12", {target::X8664Register::R12, 8}},
    {"r13", {target::X8664Register::R13, 8}},
    {"r14", {target::X8664Register::R14, 8}},
    {"r15", {target::X8664Register::R15, 8}},
    {"eax", {target::X8664Register::RAX, 4}},
    {"ecx", {target::X8664Register::RCX, 4}},
    {"edx", {target::X8664Register::RDX, 4}},
    {"ebx", {target::X8664Register::RBX, 4}},
    {"esi", {target::X8664Register::RSI, 4}},
    {"edi", {target::X8664Register::RDI, 4}},
    {"esp", {target::X8664Register::RSP, 4}},
    {"ebp", {target::X8664Register::RBP, 4}},
    {"r8d", {target::X8664Register::R8, 4}},
    {"r9d", {target::X8664Register::R9, 4}},
    {"r10d", {target::X8664Register::R10, 4}},
    {"r11d", {target::X8664Register::R11, 4}},
    {"r12d", {target::X8664Register::R12, 4}},
    {"r13d", {target::X8664Register::R13, 4}},
    {"r14d", {target::X8664Register::R14, 4}},
    {"r15d", {target::X8664Register::R15, 4}},
    {"ax", {target::X8664Register::RAX, 2}},
    {"cx", {target::X8664Register::RCX, 2}},
    {"dx", {target::X8664Register::RDX, 2}},
    {"bx", {target::X8664Register::RBX, 2}},
    {"si", {target::X8664Register::RSI, 2}},
    {"di", {target::X8664Register::RDI, 2}},
    {"sp", {target::X8664Register::RSP, 2}},
    {"bp", {target::X8664Register::RBP, 2}},
    {"r8w", {target::X8664Register::R8, 2}},
    {"r9w", {target::X8664Register::R9, 2}},
    {"r10w", {target::X8664Register::R10, 2}},
    {"r11w", {target::X8664Register::R11, 2}},
    {"r12w", {target::X8664Register::R12, 2}},
    {"r13w", {target::X8664Register::R13, 2}},
    {"r14w", {target::X8664Register::R14, 2}},
    {"r15w", {target::X8664Register::R15, 2}},
    {"al", {target::X8664Register::RAX, 1}},
    {"cl", {target::X8664Register::RCX, 1}},
    {"dl", {target::X8664Register::RDX, 1}},
    {"bl", {target::X8664Register::RBX, 1}},
    {"sil", {target::X8664Register::RSI, 1}},
    {"dil", {target::X8664Register::RDI, 1}},
    {"spl", {target::X8664Register::RSP, 1}},
    {"bpl", {target::X8664Register::RBP, 1}},
    {"r8b", {target::X8664Register::R8, 1}},
    {"r9b", {target::X8664Register::R9, 1}},
    {"r10b", {target::X8664Register::R10, 1}},
    {"r11b", {target::X8664Register::R11, 1}},
    {"r12b", {target::X8664Register::R12, 1}},
    {"r13b", {target::X8664Register::R13, 1}},
    {"r14b", {target::X8664Register::R14, 1}},
    {"r15b", {target::X8664Register::R15, 1}},
};
// clang-format on

const std::unordered_map<std::string_view, target::AArch64Condition> AARCH64_COND_MAP{
    {"eq", target::AArch64Condition::EQ},
    {"ne", target::AArch64Condition::NE},
    {"hs", target::AArch64Condition::HS},
    {"lo", target::AArch64Condition::LO},
    {"hi", target::AArch64Condition::HI},
    {"ls", target::AArch64Condition::LS},
    {"ge", target::AArch64Condition::GE},
    {"lt", target::AArch64Condition::LT},
    {"gt", target::AArch64Condition::GT},
    {"le", target::AArch64Condition::LE},
};

AssemblyUtil::AssemblyUtil(target::Architecture arch) : arch{arch}, reader{std::cin} {}

WriteBuffer AssemblyUtil::assemble() {
    mcode::Function *m_func = new mcode::Function{.name = "f"};
    mcode::BasicBlockIter m_block = m_func->basic_blocks.append({.label = "b"});

    while (reader.next_line()) {
        if (std::optional<mcode::Instruction> instr = parse_line()) {
            m_block->append(*instr);
        }
    }

    mcode::Module m_mod;
    m_mod.add(m_func);

    target::TargetDescription target{
        arch,
        target::OperatingSystem::LINUX,
        target::Environment::GNU,
    };

    if (arch == target::Architecture::X86_64) {
        BinModule bin_mod = target::X8664Encoder{}.encode(m_mod);
        return std::move(bin_mod.text);
    } else if (arch == target::Architecture::AARCH64) {
        BinModule bin_mod = target::AArch64Encoder{target}.encode(m_mod);
        return std::move(bin_mod.text);
    } else {
        ASSERT_UNREACHABLE;
    }
}

std::optional<mcode::Instruction> AssemblyUtil::parse_line() {
    reader.skip_whitespace();

    if (reader.get() == '\0' || reader.get() == '#') {
        return {};
    }

    mcode::Opcode opcode = parse_opcode();
    mcode::Instruction::OperandList operands;

    while (reader.get() != '\0') {
        operands.push_back(parse_operand());
    }

    return mcode::Instruction(opcode, operands);
}

mcode::Opcode AssemblyUtil::parse_opcode() {
    std::string string;

    while (!LineBasedReader::is_whitespace(reader.get())) {
        string += reader.consume();
    }

    reader.skip_whitespace();
    return convert_opcode(string);
}

mcode::Operand AssemblyUtil::parse_operand() {
    std::string string = read_operand();

    if (arch == target::Architecture::AARCH64) {
        if (AARCH64_COND_MAP.contains(string)) {
            return mcode::Operand::from_aarch64_condition(AARCH64_COND_MAP.at(string));
        } else if (string == "sp") {
            return mcode::Operand::from_register(convert_register(string), 8);
        } else if (string[0] == 'w' || string[0] == 's') {
            return mcode::Operand::from_register(convert_register(string), 4);
        } else if (string[0] == 'x' || string[0] == 'd') {
            return mcode::Operand::from_register(convert_register(string), 8);
        }
    } else if (arch == target::Architecture::X86_64) {
        if (auto result = X86_64_GP_REG_MAP.try_find(string)) {
            mcode::Register reg = mcode::Register::from_physical(result->first);
            return mcode::Operand::from_register(reg, result->second);
        } else if (string.starts_with("xmm")) {
            unsigned n = std::stoul(string.substr(3));
            mcode::PhysicalReg p_reg = target::X8664Register::XMM0 + n;
            mcode::Register reg = mcode::Register::from_physical(p_reg);
            return mcode::Operand::from_register(reg, 8);
        }
    }

    if (string[0] == '#') {
        std::string value = string.substr(1);

        if (value.find('.') == std::string::npos) {
            return mcode::Operand::from_int_immediate(LargeInt(value));
        } else {
            return mcode::Operand::from_fp_immediate(std::stod(value));
        }
    } else if (string.starts_with("lsl")) {
        unsigned shift_start = 0;

        while (string[shift_start] != '#') {
            shift_start += 1;
        }

        std::string shift_string = string.substr(shift_start + 1);
        return mcode::Operand::from_aarch64_left_shift(std::stoul(shift_string));
    } else if (string[0] == '[') {
        unsigned index = 1;

        while (LineBasedReader::is_whitespace(string[index])) {
            index += 1;
        }
        unsigned reg_start = index;

        index += 1;
        while (!LineBasedReader::is_whitespace(string[index]) && string[index] != ']' && string[index] != ',') {
            index += 1;
        }
        unsigned reg_end = index;

        while (LineBasedReader::is_whitespace(string[index])) {
            index += 1;
        }

        target::AArch64Address addr;
        mcode::Register base = convert_register(string.substr(reg_start, reg_end - reg_start));

        if (string[index] == ']') {
            addr = target::AArch64Address::new_base(base);
        } else if (string[index] == ',') {
            index += 1;
            while (LineBasedReader::is_whitespace(string[index])) {
                index += 1;
            }

            unsigned offset_start = index;
            bool is_imm = string[index] == '#';

            index += 1;

            while (!LineBasedReader::is_whitespace(string[index]) && string[index] != ']') {
                index += 1;
            }

            unsigned offset_end = index;

            while (LineBasedReader::is_whitespace(string[index])) {
                index += 1;
            }

            ASSERT(string[index] == ']');
            index += 1;

            if (is_imm) {
                std::string offset_string = string.substr(offset_start + 1, offset_end - offset_start - 1);
                int offset = std::stol(offset_string);

                if (string[index] == '!') {
                    addr = target::AArch64Address::new_base_offset_write(base, offset);
                } else {
                    addr = target::AArch64Address::new_base_offset(base, offset);
                }
            } else {
                std::string offset_string = string.substr(offset_start, offset_end - offset_start);
                addr = target::AArch64Address::new_base_offset(base, convert_register(offset_string));
            }
        } else {
            ASSERT_UNREACHABLE;
        }

        return mcode::Operand::from_aarch64_addr(addr);
    } else {
        ASSERT_UNREACHABLE;
    }
}

mcode::Opcode AssemblyUtil::convert_opcode(const std::string &string) {
    if (arch == target::Architecture::X86_64) {
        return X86_64_OPCODE_MAP.at(string);
    } else if (arch == target::Architecture::AARCH64) {
        return AARCH64_OPCODE_MAP.at(string);
    } else {
        ASSERT_UNREACHABLE;
    }
}

mcode::Register AssemblyUtil::convert_register(const std::string &string) {
    if (string == "sp") {
        return mcode::Register::from_physical(target::AArch64Register::SP);
    } else if (string[0] == 'w' || string[0] == 'x') {
        unsigned n = std::stoul(string.substr(1));
        mcode::PhysicalReg m_reg = target::AArch64Register::R0 + n;
        return mcode::Register::from_physical(m_reg);
    } else if (string[0] == 's' || string[0] == 'd') {
        unsigned n = std::stoul(string.substr(1));
        mcode::PhysicalReg m_reg = target::AArch64Register::V0 + n;
        return mcode::Register::from_physical(m_reg);
    } else {
        ASSERT_UNREACHABLE;
    }
}

std::string AssemblyUtil::read_operand() {
    reader.skip_whitespace();

    std::string string;
    bool in_brackets = false;

    while (reader.get() != '\0') {
        if (reader.get() == '[') {
            in_brackets = true;
        } else if (reader.get() == ']') {
            in_brackets = false;
        }

        string += reader.consume();

        if (reader.get() == ',' && !in_brackets) {
            reader.consume();
            break;
        }
    }

    unsigned length = string.size();

    while (length > 0 && LineBasedReader::is_whitespace(string[length - 1])) {
        length -= 1;
    }

    return string.substr(0, length);
}

} // namespace banjo::test
