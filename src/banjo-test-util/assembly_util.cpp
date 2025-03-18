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

    while (!is_whitespace()) {
        string += line[char_index];
        char_index += 1;
    }

    skip_whitespace();
    return convert_opcode(string);
}

mcode::Operand AssemblyUtil::parse_operand() {
    std::string string;

    while (!is_whitespace() && line[char_index] != ',') {
        string += line[char_index];
        char_index += 1;
    }

    skip_whitespace();

    if (line[char_index] == ',') {
        char_index += 1;
    }

    skip_whitespace();

    if (string[0] == 'w') {
        unsigned n = std::stoul(string.substr(1));
        mcode::PhysicalReg m_reg = target::AArch64Register::R0 + n;
        return mcode::Operand::from_register(mcode::Register::from_physical(m_reg), 4);
    } else {
        ASSERT_UNREACHABLE;
    }
}

mcode::Opcode AssemblyUtil::convert_opcode(const std::string &string) {
    if (string == "add") return target::AArch64Opcode::ADD;
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

bool AssemblyUtil::is_whitespace() {
    char c = line[char_index];
    return c == ' ' || c == '\t' || c == '\0';
}

} // namespace test
} // namespace banjo
