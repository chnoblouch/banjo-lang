#include "parser.hpp"

#include "banjo/mcode/basic_block.hpp"
#include "banjo/mcode/operand.hpp"
#include "banjo/utils/generic_lexer.hpp"
#include "banjo/utils/linked_list.hpp"

#include <iostream>
#include <optional>

namespace banjo::mcode {

static std::string token_to_string(utils::Token &token) {
    if (token.type == utils::TokenType::END_OF_LINE) {
        return "end of line";
    } else if (token.type == utils::TokenType::END_OF_FILE) {
        return "end of file";
    } else {
        return '\'' + std::string{token.value} + '\'';
    }
}

Parser::Parser(utils::TokenStream &tokens) : tokens{tokens} {}

Function *Parser::parse_func() {
    if (tokens.is_current(utils::TokenType::IDENTIFIER) && tokens.get().value == "func") {
        tokens.advance();
    } else {
        report_unexpected("'func'");
        return {};
    }

    std::string name;

    if (tokens.is_current(utils::TokenType::IDENTIFIER)) {
        name = tokens.get().value;
        tokens.advance();
    } else {
        report_unexpected("function name");
        return {};
    }

    if (tokens.is_current(utils::TokenType::COLON)) {
        tokens.advance();
    } else {
        report_unexpected("':'");
        return {};
    }

    LinkedList<BasicBlock> blocks;

    while (true) {
        if (tokens.is_current(utils::TokenType::END_OF_FILE)) {
            break;
        }

        operand_context.block_index = blocks.get_size();
        bool is_entry = blocks.get_size() == 0;

        if (std::optional<BasicBlock> block = parse_block(is_entry)) {
            BasicBlockIter iter = blocks.append(*std::move(block));
            blocks_by_name.insert(iter->label, iter);
        } else {
            return nullptr;
        }
    }

    mcode::Function *m_func = new mcode::Function{
        .name = name,
        .basic_blocks = std::move(blocks),
    };

    if (!resolve_idents(operand_context, *m_func)) {
        delete m_func;
        return nullptr;
    }

    return m_func;
}

std::optional<BasicBlock> Parser::parse_block(bool is_entry) {
    std::string label;

    if (!is_entry) {
        if (tokens.is_current(utils::TokenType::IDENTIFIER)) {
            label = tokens.get().value;
            tokens.advance();
        } else {
            report_unexpected("identifier");
            return {};
        }

        if (tokens.is_current(utils::TokenType::COLON)) {
            tokens.advance();
        } else {
            report_unexpected("':'");
            return {};
        }
    }

    BasicBlock block{.label = label};

    while (true) {
        if (tokens.is_current(utils::TokenType::END_OF_LINE)) {
            tokens.advance();
            continue;
        } else if (tokens.is_current(utils::TokenType::END_OF_FILE)) {
            break;
        }

        if (tokens.is_current(utils::TokenType::IDENTIFIER) && tokens.is_next(utils::TokenType::COLON)) {
            break;
        }

        operand_context.instr_index = block.instrs.get_size();

        std::optional<Instruction> instr = parse_instr();
        if (!instr) {
            return {};
        }

        block.append(*std::move(instr));
    }

    return block;
}

std::optional<Instruction> Parser::parse_instr() {
    std::optional<Opcode> opcode = parse_opcode();
    if (!opcode) {
        return {};
    }

    if (tokens.get().type == utils::TokenType::END_OF_LINE) {
        tokens.advance();
        return Instruction{*opcode};
    } else if (tokens.get().type == utils::TokenType::END_OF_FILE) {
        return Instruction{*opcode};
    }

    std::vector<Operand> operands;

    while (true) {
        operand_context.operand_index = operands.size();

        if (std::optional<Operand> operand = parse_operand()) {
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
            report_unexpected("comma or end of line");
            return {};
        }
    }

    return Instruction{*opcode, std::move(operands)};
}

bool Parser::resolve_idents(OperandContext context, Function &func) {
    context.block_index = 0;

    for (BasicBlock &block : func.basic_blocks) {
        context.instr_index = 0;

        for (Instruction &instr : block.instrs) {
            if (!resolve_idents(context, instr)) {
                return false;
            }

            context.instr_index += 1;
        }

        context.block_index += 1;
    }

    return true;
}

bool Parser::resolve_idents(OperandContext context, Instruction &instr) {
    for (unsigned i = 0; i < instr.get_operands().size(); i++) {
        Operand &operand = instr.get_operand(i);
        context.operand_index = i;

        for (const auto &[placeholder_context, block_name] : unresolved_blocks) {
            if (placeholder_context != context) {
                continue;
            }

            if (BasicBlockIter *block = blocks_by_name.try_find(block_name)) {
                operand = Operand::from_basic_block(**block, operand.get_size());
            } else {
                report_error("cannot find block '" + block_name + "'");
                return false;
            }
        }
    }

    return true;
}

std::optional<Operand> Parser::parse_ident_operand() {
    if (!tokens.is_current(utils::TokenType::IDENTIFIER)) {
        report_unexpected("identifier");
        return {};
    }

    unresolved_blocks.push_back({operand_context, std::string{tokens.get().value}});
    tokens.advance();
    return mcode::Operand::from_int_immediate(0);
}

void Parser::report_unexpected(const std::string &expected) {
    report_error("expected " + expected + ", got " + token_to_string(tokens.get()));
}

void Parser::report_unexpected() {
    report_error("unexpected token " + token_to_string(tokens.get()));
}

void Parser::report_error(const std::string &message) {
    std::cerr << "error: " << message << '\n';
}

} // namespace banjo::mcode
