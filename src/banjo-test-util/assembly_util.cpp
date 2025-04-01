#include "assembly_util.hpp"

#include "banjo/emit/binary_module.hpp"
#include "banjo/mcode/function.hpp"
#include "banjo/mcode/module.hpp"
#include "banjo/mcode/register.hpp"
#include "banjo/target/aarch64/aarch64_address.hpp"
#include "banjo/target/aarch64/aarch64_encoder.hpp"
#include "banjo/target/aarch64/aarch64_opcode.hpp"
#include "banjo/target/aarch64/aarch64_register.hpp"
#include "banjo/utils/macros.hpp"

#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>

namespace banjo {
namespace test {

std::unordered_map<std::string_view, mcode::Opcode> AARCH64_OPCODE_MAP = {
    {"mov", target::AArch64Opcode::MOV},       {"movz", target::AArch64Opcode::MOVZ},
    {"movk", target::AArch64Opcode::MOVK},     {"ldr", target::AArch64Opcode::LDR},
    {"ldrb", target::AArch64Opcode::LDRB},     {"ldrh", target::AArch64Opcode::LDRH},
    {"str", target::AArch64Opcode::STR},       {"strb", target::AArch64Opcode::STRB},
    {"strh", target::AArch64Opcode::STRH},     {"ldp", target::AArch64Opcode::LDP},
    {"stp", target::AArch64Opcode::STP},       {"add", target::AArch64Opcode::ADD},
    {"sub", target::AArch64Opcode::SUB},       {"mul", target::AArch64Opcode::MUL},
    {"sdiv", target::AArch64Opcode::SDIV},     {"udiv", target::AArch64Opcode::UDIV},
    {"and", target::AArch64Opcode::AND},       {"orr", target::AArch64Opcode::ORR},
    {"eor", target::AArch64Opcode::EOR},       {"lsl", target::AArch64Opcode::LSL},
    {"asr", target::AArch64Opcode::ASR},       {"csel", target::AArch64Opcode::CSEL},
    {"fmov", target::AArch64Opcode::FMOV},     {"fadd", target::AArch64Opcode::FADD},
    {"fsub", target::AArch64Opcode::FSUB},     {"fmul", target::AArch64Opcode::FMUL},
    {"fdiv", target::AArch64Opcode::FDIV},     {"fcvt", target::AArch64Opcode::FCVT},
    {"scvtf", target::AArch64Opcode::SCVTF},   {"ucvtf", target::AArch64Opcode::UCVTF},
    {"fcvtzs", target::AArch64Opcode::FCVTZS}, {"fcvtzu", target::AArch64Opcode::FCVTZU},
    {"fcsel", target::AArch64Opcode::FCSEL},   {"cmp", target::AArch64Opcode::CMP},
    {"fcmp", target::AArch64Opcode::FCMP},     {"b", target::AArch64Opcode::B},
    {"br", target::AArch64Opcode::BR},         {"b.eq", target::AArch64Opcode::B_EQ},
    {"b.ne", target::AArch64Opcode::B_NE},     {"b.hs", target::AArch64Opcode::B_HS},
    {"b.lo", target::AArch64Opcode::B_LO},     {"b.hi", target::AArch64Opcode::B_HI},
    {"b.ls", target::AArch64Opcode::B_LS},     {"b.ge", target::AArch64Opcode::B_GE},
    {"b.lt", target::AArch64Opcode::B_LT},     {"b.gt", target::AArch64Opcode::B_GT},
    {"b.le", target::AArch64Opcode::B_LE},     {"bl", target::AArch64Opcode::BL},
    {"blr", target::AArch64Opcode::BLR},       {"ret", target::AArch64Opcode::RET},
    {"adrp", target::AArch64Opcode::ADRP},     {"uxtb", target::AArch64Opcode::UXTB},
    {"uxth", target::AArch64Opcode::UXTH},     {"sxtb", target::AArch64Opcode::SXTB},
    {"sxth", target::AArch64Opcode::SXTH},     {"sxtw", target::AArch64Opcode::SXTW},
};

WriteBuffer AssemblyUtil::assemble(std::string source) {
    std::stringstream stream(std::move(source));

    mcode::Function *m_func = new mcode::Function("f", nullptr);
    mcode::BasicBlockIter m_block = m_func->get_basic_blocks().append({"b", m_func});

    while (std::getline(stream, line)) {
        char_index = 0;

        if (std::optional<mcode::Instruction> instr = parse_line()) {
            m_block->append(*instr);
        }
    }

    mcode::Module m_mod;
    m_mod.add(m_func);
    BinModule bin_mod = target::AArch64Encoder().encode(m_mod);

    return std::move(bin_mod.text);
}

std::optional<mcode::Instruction> AssemblyUtil::parse_line() {
    skip_whitespace();

    if (line[char_index] == '\0' || line[char_index] == '#') {
        return {};
    }

    mcode::Opcode opcode = parse_opcode();
    mcode::Instruction::OperandList operands;

    while (line[char_index] != '\0') {
        operands.append(parse_operand());
    }

    return mcode::Instruction(opcode, operands);
}

mcode::Opcode AssemblyUtil::parse_opcode() {
    std::string string;

    while (!is_whitespace(line[char_index])) {
        string += line[char_index];
        char_index += 1;
    }

    skip_whitespace();
    return convert_opcode(string);
}

mcode::Operand AssemblyUtil::parse_operand() {
    std::string string = read_operand();

    if (string == "sp") {
        return mcode::Operand::from_register(convert_register(string), 8);
    } else if (string[0] == 'w' || string[0] == 's') {
        return mcode::Operand::from_register(convert_register(string), 4);
    } else if (string[0] == 'x' || string[0] == 'd') {
        return mcode::Operand::from_register(convert_register(string), 8);
    } else if (string[0] == '#') {
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

        while (is_whitespace(string[index])) {
            index += 1;
        }
        unsigned reg_start = index;

        index += 1;
        while (!is_whitespace(string[index]) && string[index] != ']' && string[index] != ',') {
            index += 1;
        }
        unsigned reg_end = index;

        while (is_whitespace(string[index])) {
            index += 1;
        }

        target::AArch64Address addr;
        mcode::Register base = convert_register(string.substr(reg_start, reg_end - reg_start));

        if (string[index] == ']') {
            addr = target::AArch64Address::new_base(base);
        } else if (string[index] == ',') {
            index += 1;
            while (is_whitespace(string[index])) {
                index += 1;
            }
            unsigned offset_start = index;

            ASSERT(string[index] == '#');
            index += 1;

            while (string[index] == '-' || (string[index] >= '0' && string[index] <= '9')) {
                index += 1;
            }
            unsigned offset_end = index;

            while (is_whitespace(string[index])) {
                index += 1;
            }

            ASSERT(string[index] == ']');
            index += 1;

            int offset = std::stol(string.substr(offset_start + 1, offset_end - offset_start - 1));

            if (string[index] == '!') {
                addr = target::AArch64Address::new_base_offset_write(base, offset);
            } else {
                addr = target::AArch64Address::new_base_offset(base, offset);
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
    return AARCH64_OPCODE_MAP.at(string);
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
    skip_whitespace();

    std::string string;
    bool in_brackets = false;

    while (line[char_index] != '\0') {
        if (line[char_index] == '[') {
            in_brackets = true;
        } else if (line[char_index] == ']') {
            in_brackets = false;
        }

        string += line[char_index];
        char_index += 1;

        if (line[char_index] == ',' && !in_brackets) {
            char_index += 1;
            break;
        }
    }

    unsigned length = string.size();

    while (length > 0 && is_whitespace(string[length - 1])) {
        length -= 1;
    }

    return string.substr(0, length);
}

void AssemblyUtil::skip_whitespace() {
    if (line[char_index] == '\0') {
        return;
    }

    while (line[char_index] == ' ' || line[char_index] == '\t') {
        char_index += 1;

        if (line[char_index] == '\0') {
            return;
        }
    }
}

bool AssemblyUtil::is_whitespace(char c) {
    return c == ' ' || c == '\t' || c == '\0';
}

} // namespace test
} // namespace banjo
