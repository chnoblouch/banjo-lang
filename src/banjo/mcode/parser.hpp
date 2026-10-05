#ifndef BANJO_MCODE_PARSER_H
#define BANJO_MCODE_PARSER_H

#include "banjo/mcode/instruction.hpp"
#include "banjo/utils/generic_lexer.hpp"

#include <optional>

namespace banjo::mcode {

class Parser {

protected:
    utils::TokenStream &tokens;

public:
    Parser(utils::TokenStream &tokens);
    virtual ~Parser() = default;

    std::optional<mcode::Instruction> parse_instr();

protected:
    virtual std::optional<mcode::Opcode> parse_opcode() = 0;
    virtual std::optional<mcode::Operand> parse_operand() = 0;
};

} // namespace banjo::mcode

#endif
