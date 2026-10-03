#ifndef BANJO_TEST_UTIL_X86_64_ASSEMBLY_PARSER_H
#define BANJO_TEST_UTIL_X86_64_ASSEMBLY_PARSER_H

#include "assembler_lexer.hpp"
#include "banjo/mcode/instruction.hpp"

#include <optional>

namespace banjo::test::assembler {

class X8664AsmParser {

private:
    TokenStream &tokens;

public:
    X8664AsmParser(TokenStream &tokens);
    std::optional<mcode::Instruction> parse_instr();

private:
    std::optional<mcode::Opcode> parse_opcode();
    std::optional<mcode::Operand> parse_operand();
};

} // namespace banjo::test::assembler

#endif
