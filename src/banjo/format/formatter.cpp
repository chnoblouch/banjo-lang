#include "formatter.hpp"

#include "banjo/ast/ast_node.hpp"
#include "banjo/lexer/lexer.hpp"
#include "banjo/lexer/token.hpp"
#include "banjo/parser/parser.hpp"
#include <span>

namespace banjo::lang {

std::vector<Formatter::Edit> Formatter::format(SourceFile &file) {
    Lexer lexer{file, Lexer::Mode::FORMATTING};
    tokens = lexer.tokenize();

    Parser parser{file, tokens, Parser::Mode::FORMATTING};
    std::unique_ptr<ASTModule> ast_mod = parser.parse_module();

    if (!ast_mod->is_valid) {
        return {};
    }

    format_mod(ast_mod.get());

    return edits;
}

void Formatter::format_mod(ASTNode *node) {
    ASTNode *block_node = node->first_child;

    format_decl_block(block_node);
}

void Formatter::format_decl_block(ASTNode *node) {
    for (ASTNode *child = node->first_child; child; child = child->next_sibling) {
        if (child->type == AST_CONSTANT) {
            format_const(child);
        }
    }
}

void Formatter::format_const(ASTNode *node) {
    unsigned tkn_const = node->tokens[0];
    unsigned tkn_colon = node->tokens[1];
    unsigned tkn_equals = node->tokens[2];

    ensure_space_after(tkn_const);
    ensure_no_space_after(tkn_colon - 1);
    ensure_space_after(tkn_colon);
    ensure_space_after(tkn_equals - 1);
    ensure_space_after(tkn_equals);
}

void Formatter::ensure_no_space_after(unsigned token_index) {
    std::span<Token> attached_tokens = tokens.get_attached_tokens(token_index);

    for (const Token &attached_token : attached_tokens) {
        if (attached_token.is(TKN_WHITESPACE) || attached_token.is(TKN_LINE_ENDING)) {
            edits.push_back(Edit{.range = attached_token.range(), .replacement = ""});
        }
    }
}

void Formatter::ensure_space_after(unsigned token_index) {
    const Token &token = tokens.tokens[token_index];
    std::span<Token> attached_tokens = tokens.get_attached_tokens(token_index);

    if (attached_tokens.size() == 1) {
        if (attached_tokens[0].is(TKN_WHITESPACE) && attached_tokens[0].value == " ") {
            return;
        }
    }

    for (const Token &attached_token : attached_tokens) {
        if (attached_token.is(TKN_WHITESPACE) || attached_token.is(TKN_LINE_ENDING)) {
            edits.push_back(Edit{.range = attached_token.range(), .replacement = ""});
        }
    }

    edits.push_back(Edit{.range{token.end(), token.end()}, .replacement = " "});
}

} // namespace banjo::lang
