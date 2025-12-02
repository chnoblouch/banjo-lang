#include "formatter.hpp"

#include "banjo/ast/ast_node.hpp"
#include "banjo/lexer/lexer.hpp"
#include "banjo/lexer/token.hpp"
#include "banjo/parser/parser.hpp"
#include "banjo/utils/macros.hpp"

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

void Formatter::format_in_place(SourceFile &file) {
    std::vector<Edit> edits = format(file);

    std::string_view input = file.get_content();
    std::string output;

    if (edits.empty()) {
        output = input;
    } else {
        for (unsigned i = 0; i < edits.size(); i++) {
            unsigned prev_end = i == 0 ? 0 : edits[i - 1].range.end;
            unsigned next_start = edits[i].range.start;
            std::string_view replacement = edits[i].replacement;

            output.insert(output.end(), input.begin() + prev_end, input.begin() + next_start);
            output.insert(output.end(), replacement.begin(), replacement.end());
        }

        unsigned last_end = edits.back().range.end;
        output.insert(output.end(), input.begin() + last_end, input.end());
    }

    file.update_content(std::move(output));
}

void Formatter::format_node(ASTNode *node, WhitespaceKind whitespace) {
    switch (node->type) {
        case AST_BLOCK: format_block(node, whitespace); break;
        case AST_IDENTIFIER: format_single_token_node(node, whitespace); break;
        case AST_FUNCTION_DEFINITION: format_func_def(node, whitespace); break;
        case AST_CONSTANT: format_const_def(node, whitespace); break;
        case AST_STRUCT_DEFINITION: format_struct_def(node, whitespace); break;
        case AST_INT_LITERAL:
        case AST_FLOAT_LITERAL:
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
        case AST_VOID: format_single_token_node(node, whitespace); break;
        case AST_PARAM_LIST: format_param_list(node, whitespace); break;
        case AST_PARAM: format_param(node, whitespace); break;
    }
}

void Formatter::format_mod(ASTNode *node) {
    ASTNode *block_node = node->first_child;

    for (ASTNode *child = block_node->first_child; child; child = child->next_sibling) {
        if (child == block_node->first_child) {
            format_node(child, WhitespaceKind::INDENT_FIRST);
        } else {
            format_node(child, WhitespaceKind::INDENT);
        }
    }
}

void Formatter::format_func_def(ASTNode *node, WhitespaceKind whitespace) {
    if (whitespace == WhitespaceKind::INDENT && node->next_sibling) {
        whitespace = WhitespaceKind::INDENT_EMPTY_LINE;
    }

    ASTNode *qualifiers_node = node->first_child;
    ASTNode *name_node = qualifiers_node->next_sibling;
    ASTNode *params_node = name_node->next_sibling;
    ASTNode *return_type_node = params_node->next_sibling;
    ASTNode *block_node = return_type_node->next_sibling;

    unsigned tkn_func = node->tokens[0];
    unsigned tkn_ident = name_node->tokens[0];

    ensure_space_after(tkn_func);
    ensure_no_space_after(tkn_ident);
    format_param_list(params_node, WhitespaceKind::SPACE);

    if (return_type_node->type != AST_EMPTY) {
        unsigned tkn_arrow = node->tokens[1];

        ensure_space_after(tkn_arrow);
        format_node(return_type_node, WhitespaceKind::SPACE);
    }

    format_node(block_node, whitespace);
}

void Formatter::format_const_def(ASTNode *node, WhitespaceKind whitespace) {
    if (whitespace == WhitespaceKind::INDENT) {
        if (node->next_sibling && node->next_sibling->type != AST_CONSTANT) {
            whitespace = WhitespaceKind::INDENT_EMPTY_LINE;
        }
    }

    ASTNode *name_node = node->first_child;
    ASTNode *type_node = name_node->next_sibling;
    ASTNode *value_node = type_node->next_sibling;

    unsigned tkn_const = node->tokens[0];
    unsigned tkn_ident = name_node->tokens[0];
    unsigned tkn_colon = node->tokens[1];
    unsigned tkn_equals = node->tokens[2];

    ensure_space_after(tkn_const);
    ensure_no_space_after(tkn_ident);
    ensure_space_after(tkn_colon);
    format_node(type_node, WhitespaceKind::SPACE);
    ensure_space_after(tkn_equals);

    if (node->tokens.size() == 4) {
        unsigned tkn_semi = node->tokens[3];
        format_node(value_node, WhitespaceKind::NONE);
        ensure_whitespace_after(tkn_semi, whitespace);
    } else {
        format_node(value_node, whitespace);
    }
}

void Formatter::format_struct_def(ASTNode *node, WhitespaceKind whitespace) {
    if (whitespace == WhitespaceKind::INDENT && node->next_sibling) {
        whitespace = WhitespaceKind::INDENT_EMPTY_LINE;
    }

    ASTNode *name_node = node->first_child;
    ASTNode *impls_node = name_node->next_sibling;
    ASTNode *block_node = impls_node->next_sibling;

    format_node(block_node, whitespace);
}

void Formatter::format_block(ASTNode *node, WhitespaceKind whitespace) {
    unsigned tkn_lbrace = node->tokens[0];
    unsigned tkn_rbrace = node->tokens[1];

    indentation += 1;

    ensure_whitespace_after(tkn_lbrace, node->has_children() ? WhitespaceKind::INDENT_FIRST : WhitespaceKind::NONE);

    for (ASTNode *child = node->first_child; child; child = child->next_sibling) {
        format_node(child, child->next_sibling ? WhitespaceKind::INDENT : WhitespaceKind::INDENT_OUT);
    }

    indentation -= 1;

    ensure_whitespace_after(tkn_rbrace, whitespace);
}

void Formatter::format_param_list(ASTNode *node, WhitespaceKind whitespace) {
    format_list(node, whitespace);
}

void Formatter::format_param(ASTNode *node, WhitespaceKind whitespace) {
    ASTNode *name_node = node->first_child;
    ASTNode *expr_node = name_node->next_sibling;

    unsigned tkn_ident = name_node->tokens[0];
    unsigned tkn_colon = node->tokens[0];

    ensure_no_space_after(tkn_ident);
    ensure_space_after(tkn_colon);
    format_node(expr_node, whitespace);
}

void Formatter::format_single_token_node(ASTNode *node, WhitespaceKind whitespace) {
    unsigned token = node->tokens[0];
    ensure_whitespace_after(token, whitespace);
}

void Formatter::format_list(ASTNode *node, WhitespaceKind whitespace) {
    unsigned tkn_start = node->tokens.front();
    unsigned tkn_end = node->tokens.back();

    if (tokens.tokens[tkn_end - 1].is(TKN_COMMA)) {
        indentation += 1;

        for (unsigned i = 0; i < node->tokens.size() - 2; i++) {
            ensure_indent_after(node->tokens[i]);
        }

        ensure_indent_after(node->tokens[node->tokens.size() - 2], -1);

        indentation -= 1;
    } else {
        ensure_no_space_after(tkn_start);

        for (unsigned i = 1; i < node->tokens.size() - 1; i++) {
            ensure_space_after(node->tokens[i]);
        }

        ensure_whitespace_after(tkn_end, whitespace);
    }

    for (ASTNode *child = node->first_child; child; child = child->next_sibling) {
        format_node(child, WhitespaceKind::NONE);
    }
}

void Formatter::ensure_whitespace_after(unsigned token_index, WhitespaceKind whitespace) {
    switch (whitespace) {
        case WhitespaceKind::NONE: ensure_no_space_after(token_index); break;
        case WhitespaceKind::SPACE: ensure_space_after(token_index); break;
        case WhitespaceKind::INDENT:
        case WhitespaceKind::INDENT_FIRST:
        case WhitespaceKind::INDENT_OUT:
        case WhitespaceKind::INDENT_EMPTY_LINE: ensure_indent_after(token_index, whitespace); break;
    }
}

void Formatter::ensure_no_space_after(unsigned token_index) {
    std::span<Token> attached_tokens = tokens.get_attached_tokens(token_index + 1);

    for (const Token &attached_token : attached_tokens) {
        if (attached_token.is(TKN_WHITESPACE)) {
            edits.push_back(Edit{.range = attached_token.range(), .replacement = ""});
        }
    }
}

void Formatter::ensure_space_after(unsigned token_index) {
    ensure_whitespace_after(token_index, " ");
}

void Formatter::ensure_indent_after(unsigned token_index, WhitespaceKind whitespace) {
    std::span<Token> attached_tokens = tokens.get_attached_tokens(token_index + 1);

    if (whitespace == WhitespaceKind::INDENT) {
        unsigned num_newlines = 0;

        for (Token &token : attached_tokens) {
            if (token.is(TKN_WHITESPACE)) {
                if (token.value == "\n") {
                    num_newlines += 1;
                }
            } else {
                break;
            }
        }

        if (num_newlines >= 2) {
            whitespace = WhitespaceKind::INDENT_EMPTY_LINE;
        }
    }

    std::string whitespace_string;

    switch (whitespace) {
        case WhitespaceKind::INDENT:
        case WhitespaceKind::INDENT_FIRST: whitespace_string = "\n" + build_indent(indentation); break;
        case WhitespaceKind::INDENT_OUT: whitespace_string = "\n" + build_indent(indentation - 1); break;
        case WhitespaceKind::INDENT_EMPTY_LINE: whitespace_string = "\n\n" + build_indent(indentation); break;
        default: ASSERT_UNREACHABLE;
    }

    ensure_whitespace_after(token_index, whitespace_string);
}

void Formatter::ensure_indent_after(unsigned token_index, int indent_addend /* = 0 */) {
    unsigned num_spaces = 4 * (indentation + indent_addend);
    std::string indent_whitespace = std::string(num_spaces, ' ');
    ensure_whitespace_after(token_index, "\n" + indent_whitespace);
}

void Formatter::ensure_whitespace_after(unsigned token_index, std::string whitespace) {
    const Token &token = tokens.tokens[token_index + 1];
    std::span<Token> attached_tokens = tokens.get_attached_tokens(token_index + 1);

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

std::string Formatter::build_indent(unsigned indentation) {
    unsigned num_spaces = 4 * indentation;
    return std::string(num_spaces, ' ');
}

} // namespace banjo::lang
