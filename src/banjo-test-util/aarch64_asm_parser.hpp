#ifndef BANJO_TEST_UTIL_AARCH64_ASSEMBLY_PARSER_H
#define BANJO_TEST_UTIL_AARCH64_ASSEMBLY_PARSER_H

#include "assembler_lexer.hpp"
#include "banjo/mcode/instruction.hpp"
#include "banjo/mcode/register.hpp"
#include "banjo/target/aarch64/aarch64_address.hpp"

#include <optional>

namespace banjo::test::assembler {

class AArch64AsmParser {

private:
    TokenStream &tokens;

public:
    AArch64AsmParser(TokenStream &tokens);
    std::optional<mcode::Instruction> parse_instr();

private:
    std::optional<mcode::Opcode> parse_opcode();
    std::optional<mcode::Operand> parse_operand();

    std::optional<target::AArch64Address> parse_address();
    std::optional<mcode::Register> parse_register();
};

} // namespace banjo::test::assembler

#endif
