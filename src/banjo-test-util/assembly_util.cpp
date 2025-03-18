#include "assembly_util.hpp"

#include "banjo/emit/binary_module.hpp"
#include "banjo/mcode/function.hpp"
#include "banjo/mcode/module.hpp"
#include "banjo/mcode/register.hpp"
#include "banjo/target/aarch64/aarch64_encoder.hpp"
#include "banjo/target/aarch64/aarch64_opcode.hpp"
#include "banjo/target/aarch64/aarch64_register.hpp"
#include "banjo/utils/macros.hpp"

#include <sstream>
#include <string>
#include <utility>

namespace banjo {
namespace test {

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
    std::string string;

    while (line[char_index] != '\0') {
        string += line[char_index];
        char_index += 1;

        if (line[char_index] == ',') {
            char_index += 1;
            break;
        }
    }

    unsigned trimmed_start = 0;
    unsigned trimmed_end = 0;

    for (int i = 0; i < string.size(); i++) {
        if (!is_whitespace(string[i])) {
            trimmed_start = i;
            break;
        }
    }

    for (int i = string.size() - 1; i >= 0; i--) {
        if (!is_whitespace(string[i])) {
            trimmed_end = i;
            break;
        }
    }

    string = string.substr(trimmed_start, trimmed_end - trimmed_start + 1);

    if (string[0] == 'w') {
        unsigned n = std::stoul(string.substr(1));
        mcode::PhysicalReg m_reg = target::AArch64Register::R0 + n;
        return mcode::Operand::from_register(mcode::Register::from_physical(m_reg), 4);
    } else if (string[0] == 'x') {
        unsigned n = std::stoul(string.substr(1));
        mcode::PhysicalReg m_reg = target::AArch64Register::R0 + n;
        return mcode::Operand::from_register(mcode::Register::from_physical(m_reg), 8);
    } else if ((string[0] >= '0' && string[0] <= '9') || string[0] == '-') {
        return mcode::Operand::from_int_immediate(LargeInt(string));
    } else if (string.starts_with("lsl")) {
        unsigned shift_start = 0;

        while (!(string[shift_start] >= '0' && string[shift_start] <= '9')) {
            shift_start += 1;
        }

        std::string shift_string = string.substr(shift_start);
        return mcode::Operand::from_aarch64_left_shift(std::stoul(shift_string));
    } else {
        ASSERT_UNREACHABLE;
    }
}

mcode::Opcode AssemblyUtil::convert_opcode(const std::string &string) {
    if (string == "add") return target::AArch64Opcode::ADD;
    else if (string == "sub") return target::AArch64Opcode::SUB;
    else if (string == "ret") return target::AArch64Opcode::RET;
    else ASSERT_UNREACHABLE;
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
