#include "formatter.hpp"

#include "banjo/ast/ast_node.hpp"
#include "banjo/lexer/lexer.hpp"
#include "banjo/lexer/token.hpp"
#include "banjo/parser/parser.hpp"

#include <span>
#include <utility>

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
        switch (child->type) {
            case AST_FUNCTION_DEFINITION: format_func_def(child); break;
            case AST_CONSTANT: format_const_def(child); break;
            case AST_STRUCT_DEFINITION: format_struct_def(child); break;
            default: break;
        }
    }
}

void Formatter::format_func_def(ASTNode *node) {
    ASTNode *qualifiers_node = node->first_child;
    ASTNode *name_node = qualifiers_node->next_sibling;
    ASTNode *params_node = name_node->next_sibling;
    ASTNode *return_type_node = params_node->next_sibling;
    ASTNode *block_node = return_type_node->next_sibling;

    unsigned tkn_func = node->tokens[0];
    unsigned tkn_ident = name_node->tokens[0];

    ensure_indented(tkn_func);
    ensure_space_after(tkn_func);
    ensure_no_space_after(tkn_ident);
    format_param_list(params_node, true);

    if (return_type_node->type != AST_EMPTY) {
        unsigned tkn_arrow = node->tokens[1];

        ensure_space_after(tkn_arrow);
        format_expr(return_type_node, true);
    }

    format_block(block_node);

    first_decl = false;
}

void Formatter::format_const_def(ASTNode *node) {
    ASTNode *name_node = node->first_child;

    unsigned tkn_const = node->tokens[0];
    unsigned tkn_ident = name_node->tokens[0];
    unsigned tkn_colon = node->tokens[1];
    unsigned tkn_equals = node->tokens[2];

    ensure_indented(tkn_const);
    ensure_space_after(tkn_const);
    ensure_no_space_after(tkn_ident);
    ensure_space_after(tkn_colon);
    ensure_space_before(tkn_equals);
    ensure_space_after(tkn_equals);

    first_decl = false;
}

void Formatter::format_struct_def(ASTNode *node) {
    ASTNode *name_node = node->first_child;
    ASTNode *impls_node = name_node->next_sibling;
    ASTNode *block_node = impls_node->next_sibling;

    first_decl = false;

    indentation += 1;
    format_decl_block(block_node);
    indentation -= 1;
}

void Formatter::format_block(ASTNode *node) {
    unsigned tkn_lbrace = node->tokens[0];

    if (node->has_children()) {
        ensure_whitespace_after(tkn_lbrace, "\n");
    } else {
        ensure_no_space_after(tkn_lbrace);
    }
}

void Formatter::format_expr(ASTNode *node, bool space_after) {
    switch (node->type) {
        case AST_I8:
        case AST_I16:
        case AST_I32:
        case AST_I64:
        case AST_U8:
        case AST_U16:
        case AST_U32:
        case AST_U64:
        case AST_F32:
        case AST_F64:
        case AST_USIZE:
        case AST_BOOL:
        case AST_ADDR:
        case AST_VOID: format_keyword(node, space_after); break;
    }
}

void Formatter::format_keyword(ASTNode *node, bool space_after) {
    unsigned token = node->tokens[0];

    if (space_after) {
        ensure_space_after(token);
    } else {
        ensure_no_space_after(token);
    }
}

void Formatter::format_param_list(ASTNode *node, bool space_after) {
    unsigned tkn_lparen = node->tokens.front();
    unsigned tkn_rparen = node->tokens.back();

    ensure_no_space_after(tkn_lparen);

    for (unsigned i = 1; i < node->tokens.size() - 1; i++) {
        ensure_space_after(node->tokens[i]);
    }

    if (space_after) {
        ensure_space_after(tkn_rparen);
    } else {
        ensure_no_space_after(tkn_rparen);
    }

    for (ASTNode *child = node->first_child; child; child = child->next_sibling) {
        format_param(child);
    }
}

void Formatter::format_param(ASTNode *node) {
    ASTNode *name_node = node->first_child;
    ASTNode *expr_node = name_node->next_sibling;

    unsigned tkn_ident = name_node->tokens[0];
    unsigned tkn_colon = node->tokens[0];

    ensure_no_space_after(tkn_ident);
    ensure_space_after(tkn_colon);
    format_expr(expr_node, false);
}

void Formatter::ensure_no_space_before(unsigned token_index) {
    std::span<Token> attached_tokens = tokens.get_attached_tokens(token_index);

    for (const Token &attached_token : attached_tokens) {
        if (attached_token.is(TKN_WHITESPACE)) {
            edits.push_back(Edit{.range = attached_token.range(), .replacement = ""});
        }
    }
}

void Formatter::ensure_no_space_after(unsigned token_index) {
    ensure_no_space_before(token_index + 1);
}

void Formatter::ensure_space_before(unsigned token_index) {
    ensure_whitespace_before(token_index, " ");
}

void Formatter::ensure_space_after(unsigned token_index) {
    ensure_space_before(token_index + 1);
}

void Formatter::ensure_indented(unsigned token_index) {
    std::string indent_whitespace = std::string(4 * indentation, ' ');

    if (first_decl) {
        ensure_whitespace_before(token_index, indent_whitespace);
    } else {
        ensure_whitespace_before(token_index, "\n" + indent_whitespace);
    }
}

void Formatter::ensure_whitespace_before(unsigned token_index, std::string whitespace) {
    const Token &token = tokens.tokens[token_index];
    std::span<Token> attached_tokens = tokens.get_attached_tokens(token_index);

    if (attached_tokens.size() == 1) {
        if (attached_tokens[0].is(TKN_WHITESPACE) && attached_tokens[0].value == whitespace) {
            return;
        }
    }

    for (const Token &attached_token : attached_tokens) {
        if (attached_token.is(TKN_WHITESPACE)) {
            edits.push_back(Edit{.range = attached_token.range(), .replacement = ""});
        }
    }

    edits.push_back(Edit{.range{token.position, token.position}, .replacement = std::move(whitespace)});
}

void Formatter::ensure_whitespace_after(unsigned token_index, std::string whitespace) {
    ensure_whitespace_before(token_index + 1, std::move(whitespace));
}

} // namespace banjo::lang
