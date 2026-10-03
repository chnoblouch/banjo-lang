#ifndef BANJO_TEST_UTIL_ASSEMBLY_UTIL_H
#define BANJO_TEST_UTIL_ASSEMBLY_UTIL_H

#include "assembler_lexer.hpp"
#include "banjo/mcode/instruction.hpp"
#include "banjo/target/target_description.hpp"
#include "banjo/utils/write_buffer.hpp"

#include <optional>
#include <string_view>
#include <vector>

namespace banjo::test {

class AssemblyUtil {

private:
    target::Architecture arch;
    std::string_view source;

    std::vector<assembler::Token> tokens;
    unsigned position;

public:
    AssemblyUtil(target::Architecture arch, std::string_view source);
    WriteBuffer assemble();

private:
    std::optional<mcode::Instruction> parse_line();
    mcode::Opcode parse_opcode();
    mcode::Operand parse_operand();

    mcode::Opcode convert_opcode(std::string_view string);
    mcode::Register convert_register(std::string_view string);

    assembler::Token &get() { return tokens[position]; }
    assembler::Token &consume() { return tokens[position++]; }
};

} // namespace banjo::test

#endif
