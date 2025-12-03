#include "formatter.hpp"

#include "banjo/ast/ast_node.hpp"
#include "banjo/lexer/lexer.hpp"
#include "banjo/lexer/token.hpp"
#include "banjo/parser/parser.hpp"
#include "banjo/sir/sir.hpp"
#include "banjo/utils/macros.hpp"

#include <cstdarg>
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
        case AST_MODULE: ASSERT_UNREACHABLE;
        case AST_BLOCK: format_block(node, whitespace); break;
        case AST_IDENTIFIER: format_single_token_node(node, whitespace); break;

        case AST_FUNCTION_DEFINITION: format_func_def(node, whitespace); break;
        case AST_CONSTANT: format_const_def(node, whitespace); break;
        case AST_STRUCT_DEFINITION: format_struct_def(node, whitespace); break;
        case AST_EXPR_STMT: format_expr_stmt(node, whitespace); break;
        case AST_VAR: format_var_stmt(node, whitespace); break;
        case AST_IMPLICIT_TYPE_VAR: format_typeless_var_stmt(node, whitespace); break;
        case AST_REF_VAR: format_var_stmt(node, whitespace); break;
        case AST_IMPLICIT_TYPE_REF_VAR: format_typeless_var_stmt(node, whitespace); break;
        case AST_REF_MUT_VAR: format_var_stmt(node, whitespace); break;
        case AST_IMPLICIT_TYPE_REF_MUT_VAR: format_typeless_var_stmt(node, whitespace); break;
        case AST_ASSIGNMENT: format_assign_stmt(node, whitespace); break;
        case AST_ADD_ASSIGN: format_assign_stmt(node, whitespace); break;
        case AST_SUB_ASSIGN: format_assign_stmt(node, whitespace); break;
        case AST_MUL_ASSIGN: format_assign_stmt(node, whitespace); break;
        case AST_DIV_ASSIGN: format_assign_stmt(node, whitespace); break;
        case AST_MOD_ASSIGN: format_assign_stmt(node, whitespace); break;
        case AST_BIT_AND_ASSIGN: format_assign_stmt(node, whitespace); break;
        case AST_BIT_OR_ASSIGN: format_assign_stmt(node, whitespace); break;
        case AST_BIT_XOR_ASSIGN: format_assign_stmt(node, whitespace); break;
        case AST_SHL_ASSIGN: format_assign_stmt(node, whitespace); break;
        case AST_SHR_ASSIGN: format_assign_stmt(node, whitespace); break;
        case AST_FUNCTION_RETURN: format_return_stmt(node, whitespace); break;
        case AST_IF_CHAIN: format_if_stmt(node, whitespace); break;
        case AST_IF: format_if_branch(node, whitespace); break;
        case AST_ELSE_IF: format_else_if_branch(node, whitespace); break;
        case AST_ELSE: format_else_branch(node, whitespace); break;
        case AST_SWITCH: format_switch_stmt(node, whitespace); break;
        case AST_SWITCH_CASE_LIST: format_block(node, whitespace); break;
        case AST_SWITCH_CASE: format_switch_case_branch(node, whitespace); break;
        case AST_SWITCH_DEFAULT_CASE: ASSERT_UNREACHABLE;
        case AST_TRY: format_try_stmt(node, whitespace); break;
        case AST_TRY_SUCCESS_CASE: format_try_success_branch(node, whitespace); break;
        case AST_TRY_ERROR_CASE: format_try_except_branch(node, whitespace); break;
        case AST_TRY_ELSE_CASE: format_try_else_branch(node, whitespace); break;
        case AST_WHILE: format_while_stmt(node, whitespace); break;
        case AST_FOR: format_for_stmt(node, whitespace); break;
        case AST_FOR_REF: format_for_stmt(node, whitespace); break;
        case AST_FOR_REF_MUT: format_for_stmt(node, whitespace); break;
        case AST_CONTINUE: format_keyword_stmt(node, whitespace); break;
        case AST_BREAK: format_keyword_stmt(node, whitespace); break;
        case AST_META_IF: format_meta_if_stmt(node, whitespace); break;
        case AST_META_IF_BRANCH: format_if_branch(node, whitespace); break;
        case AST_META_ELSE_IF_BRANCH: format_else_if_branch(node, whitespace); break;
        case AST_META_ELSE_BRANCH: format_else_branch(node, whitespace); break;
        case AST_META_FOR: format_meta_for_stmt(node, whitespace); break;

        case AST_INT_LITERAL: format_single_token_node(node, whitespace); break;
        case AST_FLOAT_LITERAL: format_single_token_node(node, whitespace); break;
        case AST_CHAR_LITERAL: format_single_token_node(node, whitespace); break;
        case AST_STRING_LITERAL: format_single_token_node(node, whitespace); break;
        case AST_FALSE: format_single_token_node(node, whitespace); break;
        case AST_TRUE: format_single_token_node(node, whitespace); break;
        case AST_NULL: format_single_token_node(node, whitespace); break;
        case AST_NONE: format_single_token_node(node, whitespace); break;
        case AST_UNDEFINED: format_single_token_node(node, whitespace); break;
        case AST_ARRAY_EXPR: format_list(node, whitespace); break;
        case AST_STRUCT_INSTANTIATION: format_struct_literal(node, whitespace); break;
        case AST_ANON_STRUCT_LITERAL: format_typeless_struct_literal(node, whitespace); break;
        case AST_MAP_EXPR: ASSERT_UNREACHABLE; // TODO
        case AST_CLOSURE: ASSERT_UNREACHABLE;  // TODO
        case AST_SELF: format_single_token_node(node, whitespace); break;
        case AST_OPERATOR_ADD: format_binary_expr(node, whitespace); break;
        case AST_OPERATOR_SUB: format_binary_expr(node, whitespace); break;
        case AST_OPERATOR_MUL: format_binary_expr(node, whitespace); break;
        case AST_OPERATOR_DIV: format_binary_expr(node, whitespace); break;
        case AST_OPERATOR_MOD: format_binary_expr(node, whitespace); break;
        case AST_OPERATOR_BIT_AND: format_binary_expr(node, whitespace); break;
        case AST_OPERATOR_BIT_OR: format_binary_expr(node, whitespace); break;
        case AST_OPERATOR_BIT_XOR: format_binary_expr(node, whitespace); break;
        case AST_OPERATOR_SHL: format_binary_expr(node, whitespace); break;
        case AST_OPERATOR_SHR: format_binary_expr(node, whitespace); break;
        case AST_OPERATOR_EQ: format_binary_expr(node, whitespace); break;
        case AST_OPERATOR_NE: format_binary_expr(node, whitespace); break;
        case AST_OPERATOR_GT: format_binary_expr(node, whitespace); break;
        case AST_OPERATOR_LT: format_binary_expr(node, whitespace); break;
        case AST_OPERATOR_GE: format_binary_expr(node, whitespace); break;
        case AST_OPERATOR_LE: format_binary_expr(node, whitespace); break;
        case AST_OPERATOR_AND: format_binary_expr(node, whitespace); break;
        case AST_OPERATOR_OR: format_binary_expr(node, whitespace); break;
        case AST_OPERATOR_NEG: format_unary_expr(node, whitespace); break;
        case AST_OPERATOR_BIT_NOT: format_unary_expr(node, whitespace); break;
        case AST_OPERATOR_REF: format_unary_expr(node, whitespace); break;
        case AST_STAR_EXPR: format_unary_expr(node, whitespace); break;
        case AST_OPERATOR_NOT: format_unary_expr(node, whitespace); break;
        case AST_CAST: format_binary_expr(node, whitespace); break;
        case AST_FUNCTION_CALL: format_call_or_bracket_expr(node, whitespace); break;
        case AST_DOT_OPERATOR: format_binary_expr(node, whitespace, false); break;
        case AST_IMPLICIT_DOT_OPERATOR: format_unary_expr(node, whitespace); break;
        case AST_ARRAY_ACCESS: format_call_or_bracket_expr(node, whitespace); break;
        case AST_RANGE: format_binary_expr(node, whitespace, false); break;
        case AST_TUPLE_EXPR: ASSERT_UNREACHABLE; // TODO
        case AST_I8: format_single_token_node(node, whitespace); break;
        case AST_I16: format_single_token_node(node, whitespace); break;
        case AST_I32: format_single_token_node(node, whitespace); break;
        case AST_I64: format_single_token_node(node, whitespace); break;
        case AST_U8: format_single_token_node(node, whitespace); break;
        case AST_U16: format_single_token_node(node, whitespace); break;
        case AST_U32: format_single_token_node(node, whitespace); break;
        case AST_U64: format_single_token_node(node, whitespace); break;
        case AST_F32: format_single_token_node(node, whitespace); break;
        case AST_F64: format_single_token_node(node, whitespace); break;
        case AST_USIZE: format_single_token_node(node, whitespace); break;
        case AST_BOOL: format_single_token_node(node, whitespace); break;
        case AST_ADDR: format_single_token_node(node, whitespace); break;
        case AST_VOID: format_single_token_node(node, whitespace); break;
        case AST_STATIC_ARRAY_TYPE: format_static_array_type(node, whitespace); break;
        case AST_OPTIONAL_DATA_TYPE: format_unary_expr(node, whitespace); break;
        case AST_RESULT_TYPE: format_binary_expr(node, whitespace); break;
        case AST_CLOSURE_TYPE: ASSERT_UNREACHABLE; // TODO
        case AST_PARAM_SEQUENCE_TYPE: format_single_token_node(node, whitespace); break;
        case AST_META_EXPR: ASSERT_UNREACHABLE; // TODO

        case AST_PARAM_LIST: format_list(node, whitespace); break;
        case AST_PARAM: format_param(node, whitespace); break;
        case AST_STRUCT_FIELD_VALUE_LIST: format_list(node, whitespace, true); break;
        case AST_STRUCT_FIELD_VALUE: format_struct_literal_entry(node, whitespace); break;
        case AST_FUNCTION_ARGUMENT_LIST: format_list(node, whitespace); break;

        case AST_ERROR: ASSERT_UNREACHABLE;
        case AST_COMPLETION_TOKEN: ASSERT_UNREACHABLE;
        case AST_INVALID: ASSERT_UNREACHABLE;
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
    if (whitespace == WhitespaceKind::INDENT || whitespace == WhitespaceKind::INDENT_FIRST) {
        if (node->next_sibling) {
            whitespace = WhitespaceKind::INDENT_EMPTY_LINE;
        }
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
    format_node(params_node, WhitespaceKind::SPACE);

    if (return_type_node->type != AST_EMPTY) {
        unsigned tkn_arrow = node->tokens[1];

        ensure_space_after(tkn_arrow);
        format_node(return_type_node, WhitespaceKind::SPACE);
    }

    format_node(block_node, whitespace);
}

void Formatter::format_const_def(ASTNode *node, WhitespaceKind whitespace) {
    if (whitespace == WhitespaceKind::INDENT || whitespace == WhitespaceKind::INDENT_FIRST) {
        if (node->next_sibling && node->next_sibling->type != AST_CONSTANT) {
            whitespace = WhitespaceKind::INDENT_EMPTY_LINE;
        }
    }

    ASTNode *name_node = node->first_child;
    ASTNode *type_node = name_node->next_sibling;
    ASTNode *value_node = type_node->next_sibling;

    unsigned tkn_const = node->tokens[0];
    unsigned tkn_colon = node->tokens[1];
    unsigned tkn_equals = node->tokens[2];

    ensure_space_after(tkn_const);
    format_node(name_node, WhitespaceKind::NONE);
    ensure_space_after(tkn_colon);
    format_node(type_node, WhitespaceKind::SPACE);
    ensure_space_after(tkn_equals);
    format_before_terminator(node, value_node, whitespace, 3);
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

void Formatter::format_expr_stmt(ASTNode *node, WhitespaceKind whitespace) {
    ASTNode *expr_node = node->first_child;

    if (!node->tokens.empty()) {
        unsigned tkn_semi = node->tokens.back();
        format_node(expr_node, WhitespaceKind::NONE);
        ensure_whitespace_after(tkn_semi, whitespace);
    } else {
        format_node(expr_node, whitespace);
    }
}

void Formatter::format_var_stmt(ASTNode *node, WhitespaceKind whitespace) {
    ASTNode *name_node = node->first_child;
    ASTNode *type_node = name_node->next_sibling;
    ASTNode *value_node = type_node->next_sibling;

    if (node->type == AST_VAR || node->type == AST_REF_VAR) {
        unsigned tkn_var_or_ref = node->tokens[0];
        unsigned tkn_colon = node->tokens[1];
        unsigned tkn_equals = node->tokens[2];

        ensure_space_after(tkn_var_or_ref);
        format_node(name_node, WhitespaceKind::NONE);
        ensure_space_after(tkn_colon);
        format_node(type_node, WhitespaceKind::SPACE);
        ensure_space_after(tkn_equals);
        format_before_terminator(node, value_node, whitespace, 3);
    } else if (node->type == AST_REF_MUT_VAR) {
        unsigned tkn_ref = node->tokens[0];
        unsigned tkn_mut = node->tokens[1];
        unsigned tkn_colon = node->tokens[2];
        unsigned tkn_equals = node->tokens[3];

        ensure_space_after(tkn_ref);
        ensure_space_after(tkn_mut);
        format_node(name_node, WhitespaceKind::NONE);
        ensure_space_after(tkn_colon);
        format_node(type_node, WhitespaceKind::SPACE);
        ensure_space_after(tkn_equals);
        format_before_terminator(node, value_node, whitespace, 4);
    } else {
        ASSERT_UNREACHABLE;
    }
}

void Formatter::format_typeless_var_stmt(ASTNode *node, WhitespaceKind whitespace) {
    ASTNode *name_node = node->first_child;
    ASTNode *value_node = name_node->next_sibling;

    if (node->type == AST_IMPLICIT_TYPE_VAR || node->type == AST_IMPLICIT_TYPE_REF_VAR) {
        unsigned tkn_var_or_ref = node->tokens[0];
        unsigned tkn_equals = node->tokens[1];

        ensure_space_after(tkn_var_or_ref);
        format_node(name_node, WhitespaceKind::SPACE);
        ensure_space_after(tkn_equals);
        format_before_terminator(node, value_node, whitespace, 2);
    } else if (node->type == AST_IMPLICIT_TYPE_REF_MUT_VAR) {
        unsigned tkn_ref = node->tokens[0];
        unsigned tkn_mut = node->tokens[1];
        unsigned tkn_equals = node->tokens[2];

        ensure_space_after(tkn_ref);
        ensure_space_after(tkn_mut);
        format_node(name_node, WhitespaceKind::SPACE);
        ensure_space_after(tkn_equals);
        format_before_terminator(node, value_node, whitespace, 3);
    } else {
        ASSERT_UNREACHABLE;
    }
}

void Formatter::format_assign_stmt(ASTNode *node, WhitespaceKind whitespace) {
    ASTNode *lhs_node = node->first_child;
    ASTNode *rhs_node = lhs_node->next_sibling;

    unsigned tkn_operator = node->tokens[0];

    format_node(lhs_node, WhitespaceKind::SPACE);
    ensure_space_after(tkn_operator);
    format_before_terminator(node, rhs_node, whitespace, 1);
}

void Formatter::format_return_stmt(ASTNode *node, WhitespaceKind whitespace) {
    ASTNode *value_node = node->first_child;
    unsigned tkn_return = node->tokens[0];

    if (value_node) {
        ensure_space_after(tkn_return);
        format_before_terminator(node, value_node, whitespace, 1);
    } else {
        format_keyword_stmt(node, whitespace);
    }
}

void Formatter::format_if_stmt(ASTNode *node, WhitespaceKind whitespace) {
    for (ASTNode *child = node->first_child; child; child = child->next_sibling) {
        if (child->next_sibling) {
            format_node(child, WhitespaceKind::SPACE);
        } else {
            format_node(child, whitespace);
        }
    }
}

void Formatter::format_if_branch(ASTNode *node, WhitespaceKind whitespace) {
    ASTNode *condition_node = node->first_child;
    ASTNode *block_node = condition_node->next_sibling;

    unsigned tkn_if = node->tokens[0];

    ensure_space_after(tkn_if);
    format_node(condition_node, WhitespaceKind::SPACE);
    format_node(block_node, whitespace);
}

void Formatter::format_else_if_branch(ASTNode *node, WhitespaceKind whitespace) {
    ASTNode *condition_node = node->first_child;
    ASTNode *block_node = condition_node->next_sibling;

    unsigned tkn_else = node->tokens[0];
    unsigned tkn_if = node->tokens[1];

    ensure_space_after(tkn_else);
    ensure_space_after(tkn_if);
    format_node(condition_node, WhitespaceKind::SPACE);
    format_node(block_node, whitespace);
}

void Formatter::format_else_branch(ASTNode *node, WhitespaceKind whitespace) {
    ASTNode *block_node = node->first_child;
    unsigned tkn_else = node->tokens[0];

    ensure_space_after(tkn_else);
    format_node(block_node, whitespace);
}

void Formatter::format_switch_stmt(ASTNode *node, WhitespaceKind whitespace) {
    ASTNode *value_node = node->first_child;
    ASTNode *cases_node = value_node->next_sibling;

    unsigned tkn_switch = node->tokens[0];

    ensure_space_after(tkn_switch);
    format_node(value_node, WhitespaceKind::SPACE);
    format_node(cases_node, whitespace);
}

void Formatter::format_switch_case_branch(ASTNode *node, WhitespaceKind whitespace) {
    ASTNode *name_node = node->first_child;
    ASTNode *type_node = name_node->next_sibling;
    ASTNode *block_node = type_node->next_sibling;

    unsigned tkn_case = node->tokens[0];
    unsigned tkn_colon = node->tokens[1];

    ensure_space_after(tkn_case);
    format_node(name_node, WhitespaceKind::NONE);
    ensure_space_after(tkn_colon);
    format_node(type_node, WhitespaceKind::SPACE);
    format_node(block_node, whitespace);
}

void Formatter::format_try_stmt(ASTNode *node, WhitespaceKind whitespace) {
    for (ASTNode *child = node->first_child; child; child = child->next_sibling) {
        if (child->next_sibling) {
            format_node(child, WhitespaceKind::SPACE);
        } else {
            format_node(child, whitespace);
        }
    }
}

void Formatter::format_try_success_branch(ASTNode *node, WhitespaceKind whitespace) {
    ASTNode *name_node = node->first_child;
    ASTNode *expr_node = name_node->next_sibling;
    ASTNode *block_node = expr_node->next_sibling;

    unsigned tkn_try = node->tokens[0];
    unsigned tkn_in = node->tokens[1];

    ensure_space_after(tkn_try);
    format_node(name_node, WhitespaceKind::SPACE);
    ensure_space_after(tkn_in);
    format_node(expr_node, WhitespaceKind::SPACE);
    format_node(block_node, whitespace);
}

void Formatter::format_try_except_branch(ASTNode *node, WhitespaceKind whitespace) {
    ASTNode *name_node = node->first_child;
    ASTNode *type_node = name_node->next_sibling;
    ASTNode *block_node = type_node->next_sibling;

    unsigned tkn_except = node->tokens[0];
    unsigned tkn_colon = node->tokens[1];

    ensure_space_after(tkn_except);
    format_node(name_node, WhitespaceKind::NONE);
    ensure_space_after(tkn_colon);
    format_node(type_node, WhitespaceKind::SPACE);
    format_node(block_node, whitespace);
}

void Formatter::format_try_else_branch(ASTNode *node, WhitespaceKind whitespace) {
    ASTNode *block_node = node->first_child;
    unsigned tkn_else = node->tokens[0];

    ensure_space_after(tkn_else);
    format_node(block_node, whitespace);
}

void Formatter::format_while_stmt(ASTNode *node, WhitespaceKind whitespace) {
    ASTNode *condition_node = node->first_child;
    ASTNode *block_node = condition_node->next_sibling;

    unsigned tkn_while = node->tokens[0];

    ensure_space_after(tkn_while);
    format_node(condition_node, WhitespaceKind::SPACE);
    format_node(block_node, whitespace);
}

void Formatter::format_for_stmt(ASTNode *node, WhitespaceKind whitespace) {
    ASTNode *name_node = node->first_child;
    ASTNode *range_node = name_node->next_sibling;
    ASTNode *block_node = range_node->next_sibling;

    if (node->type == AST_FOR) {
        unsigned tkn_for = node->tokens[0];
        unsigned tkn_in = node->tokens[1];

        ensure_space_after(tkn_for);
        format_node(name_node, WhitespaceKind::SPACE);
        ensure_space_after(tkn_in);
    } else if (node->type == AST_FOR_REF) {
        unsigned tkn_for = node->tokens[0];
        unsigned tkn_ref = node->tokens[1];
        unsigned tkn_in = node->tokens[2];

        ensure_space_after(tkn_for);
        ensure_space_after(tkn_ref);
        format_node(name_node, WhitespaceKind::SPACE);
        ensure_space_after(tkn_in);
    } else if (node->type == AST_FOR_REF_MUT) {
        unsigned tkn_for = node->tokens[0];
        unsigned tkn_ref = node->tokens[1];
        unsigned tkn_ref_mut = node->tokens[2];
        unsigned tkn_in = node->tokens[3];

        ensure_space_after(tkn_for);
        ensure_space_after(tkn_ref);
        ensure_space_after(tkn_ref_mut);
        format_node(name_node, WhitespaceKind::SPACE);
        ensure_space_after(tkn_in);
    } else {
        ASSERT_UNREACHABLE;
    }

    format_node(range_node, WhitespaceKind::SPACE);
    format_node(block_node, whitespace);
}

void Formatter::format_meta_if_stmt(ASTNode *node, WhitespaceKind whitespace) {
    unsigned tkn_meta = node->tokens[0];

    ensure_space_after(tkn_meta);

    for (ASTNode *child = node->first_child; child; child = child->next_sibling) {
        if (child->next_sibling) {
            format_node(child, WhitespaceKind::SPACE);
        } else {
            format_node(child, whitespace);
        }
    }
}

void Formatter::format_meta_for_stmt(ASTNode *node, WhitespaceKind whitespace) {
    ASTNode *name_node = node->first_child;
    ASTNode *range_node = name_node->next_sibling;
    ASTNode *block_node = range_node->next_sibling;

    unsigned tkn_meta = node->tokens[0];
    unsigned tkn_for = node->tokens[1];
    unsigned tkn_in = node->tokens[2];

    ensure_space_after(tkn_meta);
    ensure_space_after(tkn_for);
    format_node(name_node, WhitespaceKind::SPACE);
    ensure_space_after(tkn_in);
    format_node(range_node, WhitespaceKind::SPACE);
    format_node(block_node, whitespace);
}

void Formatter::format_struct_literal(ASTNode *node, WhitespaceKind whitespace) {
    ASTNode *name_node = node->first_child;
    ASTNode *entries_node = name_node->next_sibling;

    format_node(name_node, WhitespaceKind::SPACE);
    format_node(entries_node, whitespace);
}

void Formatter::format_typeless_struct_literal(ASTNode *node, WhitespaceKind whitespace) {
    ASTNode *entries_node = node->first_child;

    format_node(entries_node, whitespace);
}

void Formatter::format_binary_expr(ASTNode *node, WhitespaceKind whitespace, bool spaces_between /* = true */) {
    ASTNode *lhs_node = node->first_child;
    ASTNode *rhs_node = lhs_node->next_sibling;

    unsigned tkn_operator = node->tokens[0];

    if (spaces_between) {
        format_node(lhs_node, WhitespaceKind::SPACE);
        ensure_space_after(tkn_operator);
    } else {
        format_node(lhs_node, WhitespaceKind::NONE);
        ensure_no_space_after(tkn_operator);
    }

    format_node(rhs_node, whitespace);
}

void Formatter::format_unary_expr(ASTNode *node, WhitespaceKind whitespace) {
    ASTNode *value_node = node->first_child;
    unsigned tkn_operator = node->tokens[0];

    ensure_no_space_after(tkn_operator);
    format_node(value_node, whitespace);
}

void Formatter::format_call_or_bracket_expr(ASTNode *node, WhitespaceKind whitespace) {
    ASTNode *callee_node = node->first_child;
    ASTNode *args_node = callee_node->next_sibling;

    format_node(callee_node, WhitespaceKind::NONE);
    format_node(args_node, whitespace);
}

void Formatter::format_static_array_type(ASTNode *node, WhitespaceKind whitespace) {
    ASTNode *base_type_node = node->first_child;
    ASTNode *length_node = base_type_node->next_sibling;

    unsigned tkn_lbracket = node->tokens[0];
    unsigned tkn_semi = node->tokens[1];
    unsigned tkn_rbracket = node->tokens[2];

    ensure_no_space_after(tkn_lbracket);
    format_node(base_type_node, WhitespaceKind::NONE);
    ensure_space_after(tkn_semi);
    format_node(length_node, WhitespaceKind::NONE);
    ensure_whitespace_after(tkn_rbracket, whitespace);
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

void Formatter::format_struct_literal_entry(ASTNode *node, WhitespaceKind whitespace) {
    ASTNode *name_node = node->first_child;
    ASTNode *value_node = name_node->next_sibling;

    unsigned tkn_colon = node->tokens[0];

    format_node(name_node, WhitespaceKind::NONE);
    ensure_space_after(tkn_colon);
    format_node(value_node, whitespace);
}

void Formatter::format_keyword_stmt(ASTNode *node, WhitespaceKind whitespace) {
    unsigned tkn_keyword = node->tokens[0];

    if (node->tokens.size() == 2) {
        unsigned tkn_semi = node->tokens[1];
        ensure_no_space_after(tkn_keyword);
        ensure_whitespace_after(tkn_semi, whitespace);
    } else {
        ensure_whitespace_after(tkn_keyword, whitespace);
    }
}

void Formatter::format_single_token_node(ASTNode *node, WhitespaceKind whitespace) {
    unsigned token = node->tokens[0];
    ensure_whitespace_after(token, whitespace);
}

void Formatter::format_list(ASTNode *node, WhitespaceKind whitespace, bool enclosing_spaces /* = false */) {
    unsigned tkn_start = node->tokens.front();
    unsigned tkn_end = node->tokens.back();

    if (tokens.tokens[tkn_end - 1].is(TKN_COMMA)) {
        indentation += 1;

        for (unsigned i = 0; i < node->tokens.size() - 2; i++) {
            ensure_indent_after(node->tokens[i]);
        }

        ensure_indent_after(node->tokens[node->tokens.size() - 2], -1);

        for (ASTNode *child = node->first_child; child; child = child->next_sibling) {
            format_node(child, WhitespaceKind::NONE);
        }

        indentation -= 1;
    } else {
        ensure_whitespace_after(tkn_start, enclosing_spaces ? WhitespaceKind::SPACE : WhitespaceKind::NONE);

        for (unsigned i = 1; i < node->tokens.size() - 1; i++) {
            ensure_space_after(node->tokens[i]);
        }

        for (ASTNode *child = node->first_child; child; child = child->next_sibling) {
            if (enclosing_spaces && !child->next_sibling) {
                format_node(child, WhitespaceKind::SPACE);
            } else {
                format_node(child, WhitespaceKind::NONE);
            }
        }
    }

    ensure_whitespace_after(tkn_end, whitespace);
}

void Formatter::format_before_terminator(
    ASTNode *parent,
    ASTNode *child,
    WhitespaceKind whitespace,
    unsigned semi_index
) {
    if (parent->tokens.size() == semi_index + 1) {
        unsigned tkn_semi = parent->tokens[semi_index];
        format_node(child, WhitespaceKind::NONE);
        ensure_whitespace_after(tkn_semi, whitespace);
    } else {
        format_node(child, whitespace);
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
