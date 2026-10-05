#include "parser.hpp"

#include "banjo/utils/generic_lexer.hpp"

#include <iostream>

namespace banjo::mcode {

Parser::Parser(utils::TokenStream &tokens) : tokens{tokens} {}

std::optional<mcode::Instruction> Parser::parse_instr() {
    std::optional<mcode::Opcode> opcode = parse_opcode();
    if (!opcode) {
        return {};
    }

    if (tokens.get().type == utils::TokenType::END_OF_LINE) {
        tokens.advance();
        return mcode::Instruction{*opcode};
    } else if (tokens.get().type == utils::TokenType::END_OF_FILE) {
        return mcode::Instruction{*opcode};
    }

    std::vector<mcode::Operand> operands;

    while (true) {
        if (std::optional<mcode::Operand> operand = parse_operand()) {
            operands.push_back(*std::move(operand));
        } else {
            return {};
        }

        utils::Token &token = tokens.get();

        if (token.type == utils::TokenType::COMMA) {
            tokens.advance();
        } else if (token.type == utils::TokenType::END_OF_LINE) {
            tokens.advance();
            break;
        } else if (token.type == utils::TokenType::END_OF_FILE) {
            break;
        } else {
            std::cerr << "error: expected comma or end of line, got '" + std::string{token.value} + "'\n";
            return {};
        }
    }

    return mcode::Instruction{*opcode, std::move(operands)};
}

} // namespace banjo::mcode
