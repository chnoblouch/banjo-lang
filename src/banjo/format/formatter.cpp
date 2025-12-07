#include "formatter.hpp"

#include "banjo/ast/ast_node.hpp"
#include "banjo/lexer/lexer.hpp"
#include "banjo/lexer/token.hpp"
#include "banjo/parser/parser.hpp"
#include "banjo/sir/sir.hpp"
#include "banjo/utils/macros.hpp"
#include "banjo/utils/utils.hpp"

#include <algorithm>
#include <cstdarg>
#include <span>
#include <utility>

namespace banjo::lang {

std::vector<Formatter::Edit> Formatter::format(SourceFile &file) {
    file_content = file.get_content();

    Lexer lexer{file, Lexer::Mode::KEEP_WHITESPACE};
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
        case AST_FUNCTION_DEFINITION: format_func_def(node, whitespace); break;
        case AST_GENERIC_FUNCTION_DEFINITION: format_generic_func_def(node, whitespace); break;
        case AST_FUNC_DECL: format_func_decl(node, whitespace); break;
        case AST_NATIVE_FUNCTION_DECLARATION: format_func_decl(node, whitespace); break;
        case AST_CONSTANT: format_const_def(node, whitespace); break;
        case AST_STRUCT_DEFINITION: format_struct_def(node, whitespace); break;
        case AST_GENERIC_STRUCT_DEFINITION: format_generic_struct_def(node, whitespace); break;
        case AST_ENUM_DEFINITION: format_enum_def(node, whitespace); break;
        case AST_ENUM_VARIANT_LIST: format_list(node, whitespace); break;
        case AST_ENUM_VARIANT: format_enum_variant(node, whitespace); break;
        case AST_UNION: format_union_def(node, whitespace); break;
        case AST_UNION_CASE: format_union_case(node, whitespace); break;
        case AST_PROTO: format_proto_def(node, whitespace); break;
        case AST_TYPE_ALIAS: format_type_alias(node, whitespace); break;
        case AST_NATIVE_VAR: format_var_decl(node, whitespace); break;
        case AST_USE: format_use_decl(node, whitespace); break;
        case AST_USE_TREE_LIST: format_list(node, whitespace); break;
        case AST_USE_REBINDING: format_use_rebind(node, whitespace); break;
        case AST_EXPR_STMT: format_expr_stmt(node, whitespace); break;
        case AST_VAR: global_scope ? format_var_decl(node, whitespace) : format_var_stmt(node, whitespace); break;
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
        case AST_PAREN_EXPR: format_paren_expr(node, whitespace); break;
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
        case AST_MAP_EXPR: format_list(node, whitespace); break;
        case AST_MAP_EXPR_PAIR: format_map_literal_entry(node, whitespace); break;
        case AST_CLOSURE: format_closure_literal(node, whitespace); break;
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
        case AST_TUPLE_EXPR: format_list(node, whitespace); break;
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
        case AST_FUNCTION_DATA_TYPE: format_func_type(node, whitespace); break;
        case AST_OPTIONAL_DATA_TYPE: format_unary_expr(node, whitespace); break;
        case AST_RESULT_TYPE: format_binary_expr(node, whitespace); break;
        case AST_CLOSURE_TYPE: format_closure_type(node, whitespace); break;
        case AST_PARAM_SEQUENCE_TYPE: format_single_token_node(node, whitespace); break;
        case AST_META_EXPR: format_meta_expr(node, whitespace); break;
        case AST_IDENTIFIER: format_single_token_node(node, whitespace); break;
        case AST_PARAM_LIST: format_param_list(node, whitespace); break;
        case AST_PARAM: format_param(node, whitespace); break;
        case AST_REF_PARAM: format_param(node, whitespace); break;
        case AST_REF_MUT_PARAM: format_param(node, whitespace); break;
        case AST_REF_RETURN: format_ref_return(node, whitespace); break;
        case AST_REF_MUT_RETURN: format_ref_return(node, whitespace); break;
        case AST_GENERIC_PARAM_LIST: format_list(node, whitespace); break;
        case AST_GENERIC_PARAMETER: format_generic_param(node, whitespace); break;
        case AST_STRUCT_FIELD_VALUE_LIST: format_list(node, whitespace, true); break;
        case AST_STRUCT_FIELD_VALUE: format_struct_literal_entry(node, whitespace); break;
        case AST_FUNCTION_ARGUMENT_LIST: format_list(node, whitespace); break;
        case AST_IMPL_LIST: format_impl_list(node, whitespace); break;
        case AST_QUALIFIER_LIST: format_qualifier_list(node, whitespace); break;
        case AST_QUALIFIER: format_single_token_node(node, whitespace); break;
        case AST_ATTRIBUTE_WRAPPER: format_attribute_wrapper(node, whitespace); break;
        case AST_ATTRIBUTE_LIST: format_attribute_list(node, whitespace); break;
        case AST_ATTRIBUTE_TAG: format_single_token_node(node, whitespace); break;
        case AST_ATTRIBUTE_VALUE: format_attribute_value(node, whitespace); break;
        case AST_EMPTY: ASSERT_UNREACHABLE;
        case AST_ERROR: ASSERT_UNREACHABLE;
        case AST_COMPLETION_TOKEN: ASSERT_UNREACHABLE;
        case AST_INVALID: ASSERT_UNREACHABLE;
    }
}

void Formatter::format_mod(ASTNode *node) {
    ASTNode *block_node = node->first_child;

    if (!tokens.tokens.empty()) {
        std::span<Token> first_attached_tokens = tokens.get_attached_tokens(0);

        if (!first_attached_tokens.empty() && first_attached_tokens[0].is(TKN_WHITESPACE)) {
            add_delete_edit(first_attached_tokens[0].range());
        }

        for (Token &token : first_attached_tokens) {
            if (token.is(TKN_COMMENT)) {
                format_comment_text(token);
            }
        }
    }

    for (ASTNode *child = block_node->first_child; child; child = child->next_sibling) {
        format_node(child, WhitespaceKind::INDENT);
    }
}

void Formatter::format_func_def(ASTNode *node, WhitespaceKind whitespace) {
    if (whitespace == WhitespaceKind::INDENT_ALLOW_EMPTY_LINE || whitespace == WhitespaceKind::INDENT) {
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

    format_node(qualifiers_node, WhitespaceKind::SPACE);
    ensure_space_after(tkn_func);
    format_node(name_node, WhitespaceKind::NONE);
    format_node(params_node, WhitespaceKind::SPACE);

    if (return_type_node->type != AST_EMPTY) {
        unsigned tkn_arrow = node->tokens[1];
        ensure_space_after(tkn_arrow);
        format_node(return_type_node, WhitespaceKind::SPACE);
    }

    bool was_global_scope = global_scope;
    global_scope = false;
    format_node(block_node, whitespace);
    global_scope = was_global_scope;
}

void Formatter::format_generic_func_def(ASTNode *node, WhitespaceKind whitespace) {
    if (whitespace == WhitespaceKind::INDENT_ALLOW_EMPTY_LINE || whitespace == WhitespaceKind::INDENT) {
        if (node->next_sibling) {
            whitespace = WhitespaceKind::INDENT_EMPTY_LINE;
        }
    }

    ASTNode *qualifiers_node = node->first_child;
    ASTNode *name_node = qualifiers_node->next_sibling;
    ASTNode *generic_params_node = name_node->next_sibling;
    ASTNode *params_node = generic_params_node->next_sibling;
    ASTNode *return_type_node = params_node->next_sibling;
    ASTNode *block_node = return_type_node->next_sibling;

    unsigned tkn_func = node->tokens[0];

    format_node(qualifiers_node, WhitespaceKind::SPACE);
    ensure_space_after(tkn_func);
    format_node(name_node, WhitespaceKind::NONE);
    format_node(generic_params_node, WhitespaceKind::NONE);
    format_node(params_node, WhitespaceKind::SPACE);

    if (return_type_node->type != AST_EMPTY) {
        unsigned tkn_arrow = node->tokens[1];
        ensure_space_after(tkn_arrow);
        format_node(return_type_node, WhitespaceKind::SPACE);
    }

    bool was_global_scope = global_scope;
    global_scope = false;
    format_node(block_node, whitespace);
    global_scope = was_global_scope;
}

void Formatter::format_func_decl(ASTNode *node, WhitespaceKind whitespace) {
    if (whitespace == WhitespaceKind::INDENT_ALLOW_EMPTY_LINE || whitespace == WhitespaceKind::INDENT) {
        if (node->next_sibling && node->next_sibling->type != node->type) {
            whitespace = WhitespaceKind::INDENT_EMPTY_LINE;
        }
    }

    ASTNode *qualifiers_node = node->first_child;
    ASTNode *name_node = qualifiers_node->next_sibling;
    ASTNode *params_node = name_node->next_sibling;
    ASTNode *return_type_node = params_node->next_sibling;

    format_node(qualifiers_node, WhitespaceKind::SPACE);

    unsigned tkn_index_offset = 0;

    if (node->type == AST_NATIVE_FUNCTION_DECLARATION) {
        unsigned tkn_native = node->tokens[0];
        ensure_space_after(tkn_native);
        tkn_index_offset = 1;
    }

    unsigned tkn_func = node->tokens[tkn_index_offset + 0];

    ensure_space_after(tkn_func);
    format_node(name_node, WhitespaceKind::NONE);

    if (return_type_node->type == AST_EMPTY) {
        format_before_terminator(node, params_node, whitespace, tkn_index_offset + 1);
    } else {
        unsigned tkn_arrow = node->tokens[tkn_index_offset + 1];

        format_node(params_node, WhitespaceKind::SPACE);
        ensure_space_after(tkn_arrow);
        format_before_terminator(node, return_type_node, whitespace, tkn_index_offset + 2);
    }
}

void Formatter::format_const_def(ASTNode *node, WhitespaceKind whitespace) {
    if (whitespace == WhitespaceKind::INDENT_ALLOW_EMPTY_LINE || whitespace == WhitespaceKind::INDENT) {
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
    if (whitespace == WhitespaceKind::INDENT_ALLOW_EMPTY_LINE || whitespace == WhitespaceKind::INDENT) {
        if (node->next_sibling) {
            whitespace = WhitespaceKind::INDENT_EMPTY_LINE;
        }
    }

    ASTNode *name_node = node->first_child;
    ASTNode *impls_node = name_node->next_sibling;
    ASTNode *block_node = impls_node->next_sibling;

    unsigned tkn_struct = node->tokens[0];

    ensure_space_after(tkn_struct);
    format_node(name_node, impls_node->tokens.empty() ? WhitespaceKind::SPACE : WhitespaceKind::NONE);
    format_node(impls_node, WhitespaceKind::SPACE);
    format_node(block_node, whitespace);
}

void Formatter::format_generic_struct_def(ASTNode *node, WhitespaceKind whitespace) {
    if (whitespace == WhitespaceKind::INDENT_ALLOW_EMPTY_LINE || whitespace == WhitespaceKind::INDENT) {
        if (node->next_sibling) {
            whitespace = WhitespaceKind::INDENT_EMPTY_LINE;
        }
    }

    ASTNode *name_node = node->first_child;
    ASTNode *generic_params_node = name_node->next_sibling;
    ASTNode *block_node = generic_params_node->next_sibling;

    unsigned tkn_struct = node->tokens[0];

    ensure_space_after(tkn_struct);
    format_node(name_node, WhitespaceKind::NONE);
    format_node(generic_params_node, WhitespaceKind::SPACE);
    format_node(block_node, whitespace);
}

void Formatter::format_enum_def(ASTNode *node, WhitespaceKind whitespace) {
    if (whitespace == WhitespaceKind::INDENT_ALLOW_EMPTY_LINE || whitespace == WhitespaceKind::INDENT) {
        if (node->next_sibling) {
            whitespace = WhitespaceKind::INDENT_EMPTY_LINE;
        }
    }

    ASTNode *name_node = node->first_child;
    ASTNode *variants_node = name_node->next_sibling;

    unsigned tkn_enum = node->tokens[0];

    ensure_space_after(tkn_enum);
    format_node(name_node, WhitespaceKind::SPACE);
    format_node(variants_node, whitespace); // TODO: Spaces!
}

void Formatter::format_enum_variant(ASTNode *node, WhitespaceKind whitespace) {
    ASTNode *name_node = node->first_child;
    ASTNode *value_node = name_node->next_sibling;

    if (value_node) {
        unsigned tkn_equals = node->tokens[0];

        format_node(name_node, WhitespaceKind::SPACE);
        ensure_space_after(tkn_equals);
        format_node(value_node, whitespace);
    } else {
        format_node(name_node, whitespace);
    }
}

void Formatter::format_union_def(ASTNode *node, WhitespaceKind whitespace) {
    if (whitespace == WhitespaceKind::INDENT_ALLOW_EMPTY_LINE || whitespace == WhitespaceKind::INDENT) {
        if (node->next_sibling) {
            whitespace = WhitespaceKind::INDENT_EMPTY_LINE;
        }
    }

    ASTNode *name_node = node->first_child;
    ASTNode *block_node = name_node->next_sibling;

    unsigned tkn_union = node->tokens[0];

    ensure_space_after(tkn_union);
    format_node(name_node, WhitespaceKind::SPACE);
    format_node(block_node, whitespace);
}

void Formatter::format_union_case(ASTNode *node, WhitespaceKind whitespace) {
    ASTNode *name_node = node->first_child;
    ASTNode *fields_node = name_node->next_sibling;

    unsigned tkn_case = node->tokens[0];

    ensure_space_after(tkn_case);
    format_node(name_node, WhitespaceKind::NONE);

    if (node->tokens.size() == 2) {
        unsigned tkn_semi = node->tokens[1];
        format_list(fields_node, WhitespaceKind::NONE);
        ensure_whitespace_after(tkn_semi, whitespace);
    } else {
        format_list(fields_node, whitespace);
    }
}

void Formatter::format_proto_def(ASTNode *node, WhitespaceKind whitespace) {
    if (whitespace == WhitespaceKind::INDENT_ALLOW_EMPTY_LINE || whitespace == WhitespaceKind::INDENT) {
        if (node->next_sibling) {
            whitespace = WhitespaceKind::INDENT_EMPTY_LINE;
        }
    }

    ASTNode *name_node = node->first_child;
    ASTNode *block_node = name_node->next_sibling;

    unsigned tkn_proto = node->tokens[0];

    ensure_space_after(tkn_proto);
    format_node(name_node, WhitespaceKind::SPACE);
    format_node(block_node, whitespace);
}

void Formatter::format_type_alias(ASTNode *node, WhitespaceKind whitespace) {
    if (whitespace == WhitespaceKind::INDENT_ALLOW_EMPTY_LINE || whitespace == WhitespaceKind::INDENT) {
        if (node->next_sibling && node->next_sibling->type != AST_CONSTANT) {
            whitespace = WhitespaceKind::INDENT_EMPTY_LINE;
        }
    }

    ASTNode *name_node = node->first_child;
    ASTNode *underlying_type_node = name_node->next_sibling;

    unsigned tkn_type = node->tokens[0];
    unsigned tkn_equals = node->tokens[1];

    ensure_space_after(tkn_type);
    format_node(name_node, WhitespaceKind::SPACE);
    ensure_space_after(tkn_equals);
    format_before_terminator(node, underlying_type_node, whitespace, 2);
}

void Formatter::format_use_decl(ASTNode *node, WhitespaceKind whitespace) {
    if (whitespace == WhitespaceKind::INDENT_ALLOW_EMPTY_LINE || whitespace == WhitespaceKind::INDENT) {
        if (node->next_sibling && node->next_sibling->type != AST_USE) {
            whitespace = WhitespaceKind::INDENT_EMPTY_LINE;
        }
    }

    ASTNode *root_item_node = node->first_child;
    unsigned tkn_use = node->tokens[0];

    ensure_space_after(tkn_use);
    format_before_terminator(node, root_item_node, whitespace, 1);
}

void Formatter::format_use_rebind(ASTNode *node, WhitespaceKind whitespace) {
    ASTNode *target_name_node = node->first_child;
    ASTNode *local_name_node = target_name_node->next_sibling;

    unsigned tkn_as = node->tokens[0];

    format_node(target_name_node, WhitespaceKind::SPACE);
    ensure_space_after(tkn_as);
    format_node(local_name_node, whitespace);
}

void Formatter::format_block(ASTNode *node, WhitespaceKind whitespace) {
    // TODO: Declaration and statement blocks should have different types so we know if we're in
    // the global scope or not.

    unsigned tkn_lbrace = node->tokens[0];
    unsigned tkn_rbrace = node->tokens[1];

    indentation += 1;

    if (node->has_children()) {
        ensure_whitespace_after(tkn_lbrace, WhitespaceKind::INDENT);
    } else if (followed_by_comment(tkn_lbrace)) {
        ensure_whitespace_after(tkn_lbrace, WhitespaceKind::INDENT_OUT);
    } else {
        ensure_whitespace_after(tkn_lbrace, WhitespaceKind::NONE);
    }

    for (ASTNode *child = node->first_child; child; child = child->next_sibling) {
        format_node(child, child->next_sibling ? WhitespaceKind::INDENT_ALLOW_EMPTY_LINE : WhitespaceKind::INDENT_OUT);
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

    unsigned tkn_index_offset = 0;

    if (node->type == AST_REF_MUT_VAR) {
        unsigned tkn_ref = node->tokens[0];
        ensure_space_after(tkn_ref);
        tkn_index_offset = 1;
    }

    unsigned tkn_var_or_ref = node->tokens[tkn_index_offset + 0];
    unsigned tkn_colon = node->tokens[tkn_index_offset + 1];

    ensure_space_after(tkn_var_or_ref);
    format_node(name_node, WhitespaceKind::NONE);
    ensure_space_after(tkn_colon);

    if (value_node) {
        unsigned tkn_equals = node->tokens[tkn_index_offset + 2];

        format_node(type_node, WhitespaceKind::SPACE);
        ensure_space_after(tkn_equals);
        format_before_terminator(node, value_node, whitespace, tkn_index_offset + 3);
    } else {
        format_before_terminator(node, type_node, whitespace, tkn_index_offset + 2);
    }
}

void Formatter::format_var_decl(ASTNode *node, WhitespaceKind whitespace) {
    if (whitespace == WhitespaceKind::INDENT_ALLOW_EMPTY_LINE || whitespace == WhitespaceKind::INDENT) {
        if (node->next_sibling && node->next_sibling->type != node->type) {
            whitespace = WhitespaceKind::INDENT_EMPTY_LINE;
        }
    }

    ASTNode *name_node = node->first_child;
    ASTNode *type_node = name_node->next_sibling;
    ASTNode *value_node = type_node->next_sibling;

    unsigned tkn_index_offset = 0;

    if (node->type == AST_NATIVE_VAR) {
        unsigned tkn_native = node->tokens[0];
        ensure_space_after(tkn_native);
        tkn_index_offset = 1;
    }

    unsigned tkn_var = node->tokens[tkn_index_offset + 0];
    unsigned tkn_colon = node->tokens[tkn_index_offset + 1];

    ensure_space_after(tkn_var);
    format_node(name_node, WhitespaceKind::NONE);
    ensure_space_after(tkn_colon);

    if (value_node) {
        unsigned tkn_equals = node->tokens[tkn_index_offset + 2];

        format_node(type_node, WhitespaceKind::SPACE);
        ensure_space_after(tkn_equals);
        format_before_terminator(node, value_node, whitespace, tkn_index_offset + 3);
    } else {
        format_before_terminator(node, type_node, whitespace, tkn_index_offset + 2);
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

void Formatter::format_paren_expr(ASTNode *node, WhitespaceKind whitespace) {
    ASTNode *child = node->first_child;

    unsigned tkn_lparen = node->tokens[0];
    unsigned tkn_rparen = node->tokens[1];

    ensure_no_space_after(tkn_lparen);
    format_node(child, WhitespaceKind::NONE);
    ensure_whitespace_after(tkn_rparen, whitespace);
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

void Formatter::format_map_literal_entry(ASTNode *node, WhitespaceKind whitespace) {
    ASTNode *key_node = node->first_child;
    ASTNode *value_node = key_node->next_sibling;

    unsigned tkn_colon = node->tokens[0];

    format_node(key_node, WhitespaceKind::NONE);
    ensure_space_after(tkn_colon);
    format_node(value_node, whitespace);
}

void Formatter::format_closure_literal(ASTNode *node, WhitespaceKind whitespace) {
    ASTNode *params_node = node->first_child;
    ASTNode *return_type_node = params_node->next_sibling;
    ASTNode *block_node = return_type_node->next_sibling;

    format_node(params_node, WhitespaceKind::SPACE);

    if (return_type_node->type != AST_EMPTY) {
        unsigned tkn_arrow = node->tokens[0];

        ensure_space_after(tkn_arrow);
        format_node(return_type_node, WhitespaceKind::SPACE);
    }

    bool was_global_scope = global_scope;
    global_scope = false;
    format_node(block_node, whitespace);
    global_scope = was_global_scope;
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

void Formatter::format_func_type(ASTNode *node, WhitespaceKind whitespace) {
    ASTNode *params_node = node->first_child;
    ASTNode *return_type_node = params_node->next_sibling;

    unsigned tkn_func = node->tokens[0];

    ensure_no_space_after(tkn_func);

    if (return_type_node->type == AST_EMPTY) {
        format_node(params_node, whitespace);
    } else {
        unsigned tkn_arrow = node->tokens[1];

        format_node(params_node, WhitespaceKind::SPACE);
        ensure_space_after(tkn_arrow);
        format_node(return_type_node, whitespace);
    }
}

void Formatter::format_closure_type(ASTNode *node, WhitespaceKind whitespace) {
    ASTNode *params_node = node->first_child;
    ASTNode *return_type_node = params_node->next_sibling;

    if (return_type_node->type == AST_EMPTY) {
        format_node(params_node, whitespace);
    } else {
        unsigned tkn_arrow = node->tokens[0];

        format_node(params_node, WhitespaceKind::SPACE);
        ensure_space_after(tkn_arrow);
        format_node(return_type_node, whitespace);
    }
}

void Formatter::format_meta_expr(ASTNode *node, WhitespaceKind whitespace) {
    ASTNode *expr_node = node->first_child;

    unsigned tkn_meta = node->tokens[0];
    unsigned tkn_lparen = node->tokens[1];
    unsigned tkn_rparen = node->tokens[2];

    ensure_no_space_after(tkn_meta);
    ensure_no_space_after(tkn_lparen);
    format_node(expr_node, WhitespaceKind::NONE);
    ensure_whitespace_after(tkn_rparen, whitespace);
}

void Formatter::format_param_list(ASTNode *node, WhitespaceKind whitespace) {
    unsigned first_token = node->tokens[0];

    if (tokens.tokens[first_token].is(TKN_OR_OR)) {
        ensure_whitespace_after(first_token, whitespace);
    } else {
        return format_list(node, whitespace);
    }
}

void Formatter::format_param(ASTNode *node, WhitespaceKind whitespace) {
    ASTNode *name_node = node->first_child;
    ASTNode *expr_node = name_node->next_sibling;

    if (node->type == AST_PARAM) {
        if (name_node->type == AST_EMPTY) {
            format_node(expr_node, whitespace);
        } else {
            unsigned tkn_colon = node->tokens[0];

            format_node(name_node, WhitespaceKind::NONE);
            ensure_space_after(tkn_colon);
            format_node(expr_node, whitespace);
        }
    } else if (node->type == AST_REF_PARAM) {
        if (name_node->type == AST_SELF) {
            format_node(name_node, whitespace);
        } else {
            unsigned tkn_ref = node->tokens[0];
            unsigned tkn_colon = node->tokens[1];

            ensure_space_after(tkn_ref);
            format_node(name_node, WhitespaceKind::NONE);
            ensure_space_after(tkn_colon);
            format_node(expr_node, whitespace);
        }
    } else if (node->type == AST_REF_MUT_PARAM) {
        if (name_node->type == AST_SELF) {
            unsigned tkn_mut = node->tokens[0];
            ensure_space_after(tkn_mut);
            format_node(name_node, whitespace);
        } else {
            unsigned tkn_ref = node->tokens[0];
            unsigned tkn_mut = node->tokens[1];
            unsigned tkn_colon = node->tokens[2];

            ensure_space_after(tkn_ref);
            ensure_space_after(tkn_mut);
            format_node(name_node, WhitespaceKind::NONE);
            ensure_space_after(tkn_colon);
            format_node(expr_node, whitespace);
        }
    } else {
        ASSERT_UNREACHABLE;
    }
}

void Formatter::format_ref_return(ASTNode *node, WhitespaceKind whitespace) {
    ASTNode *type = node->first_child;
    unsigned tkn_ref = node->tokens[0];

    ensure_space_after(tkn_ref);

    if (node->type == AST_REF_MUT_RETURN) {
        unsigned tkn_mut = node->tokens[1];
        ensure_space_after(tkn_mut);
    }

    format_node(type, whitespace);
}

void Formatter::format_generic_param(ASTNode *node, WhitespaceKind whitespace) {
    ASTNode *name_node = node->first_child;
    ASTNode *type_node = name_node->next_sibling;

    if (type_node) {
        unsigned tkn_colon = node->tokens[0];

        format_node(name_node, WhitespaceKind::NONE);
        ensure_space_after(tkn_colon);
        format_node(type_node, whitespace);
    } else {
        format_node(name_node, whitespace);
    }
}

void Formatter::format_struct_literal_entry(ASTNode *node, WhitespaceKind whitespace) {
    ASTNode *name_node = node->first_child;
    ASTNode *value_node = name_node->next_sibling;

    if (value_node) {
        unsigned tkn_colon = node->tokens[0];

        format_node(name_node, WhitespaceKind::NONE);
        ensure_space_after(tkn_colon);
        format_node(value_node, whitespace);
    } else {
        format_node(name_node, whitespace);
    }
}

void Formatter::format_impl_list(ASTNode *node, WhitespaceKind whitespace) {
    // TODO: What about trailing commas here?

    if (node->tokens.empty()) {
        return;
    }

    unsigned tkn_colon = node->tokens[0];
    ensure_space_after(tkn_colon);

    for (unsigned i = 1; i < node->tokens.size(); i++) {
        ensure_space_after(node->tokens[i]);
    }

    for (ASTNode *child = node->first_child; child; child = child->next_sibling) {
        if (child->next_sibling || tokens.tokens[node->tokens.back()].is(TKN_COMMA)) {
            format_node(child, WhitespaceKind::NONE);
        } else {
            format_node(child, whitespace);
        }
    }
}

void Formatter::format_qualifier_list(ASTNode *node, WhitespaceKind whitespace) {
    for (ASTNode *child = node->first_child; child; child = child->next_sibling) {
        if (child->next_sibling) {
            format_node(child, WhitespaceKind::SPACE);
        } else {
            format_node(child, whitespace);
        }
    }
}

void Formatter::format_attribute_wrapper(ASTNode *node, WhitespaceKind whitespace) {
    ASTNode *attrs_node = node->first_child;
    ASTNode *wrapped_node = attrs_node->next_sibling;

    unsigned tkn_at = node->tokens[0];

    if (Utils::is_one_of(wrapped_node->type, {AST_PARAM, AST_REF_PARAM, AST_REF_MUT_PARAM})) {
        ensure_no_space_after(tkn_at);
        format_node(attrs_node, WhitespaceKind::SPACE);
        format_node(wrapped_node, whitespace);
    } else {
        ensure_no_space_after(tkn_at);
        format_node(attrs_node, WhitespaceKind::INDENT);
        format_node(wrapped_node, WhitespaceKind::INDENT_EMPTY_LINE);
    }
}

void Formatter::format_attribute_list(ASTNode *node, WhitespaceKind whitespace) {
    if (node->tokens.empty()) {
        format_node(node->first_child, whitespace);
    } else {
        format_list(node, whitespace);
    }
}

void Formatter::format_attribute_value(ASTNode *node, WhitespaceKind whitespace) {
    ASTNode *name_node = node->first_child;
    ASTNode *value_node = name_node->next_sibling;

    unsigned tkn_equals = node->tokens[0];

    format_node(name_node, WhitespaceKind::NONE);
    ensure_no_space_after(tkn_equals);
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

    bool multiline = tokens.tokens[tkn_end - 1].is(TKN_COMMA) || followed_by_comment(tkn_start);

    if (multiline) {
        indentation += 1;

        for (unsigned i = 0; i < node->tokens.size() - 2; i++) {
            ensure_whitespace_after(node->tokens[i], WhitespaceKind::INDENT);
        }

        ensure_whitespace_after(node->tokens[node->tokens.size() - 2], WhitespaceKind::INDENT_OUT);

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

void Formatter::ensure_no_space_after(unsigned token_index) {
    ensure_whitespace_after(token_index, WhitespaceKind::NONE);
}

void Formatter::ensure_space_after(unsigned token_index) {
    ensure_whitespace_after(token_index, WhitespaceKind::SPACE);
}

void Formatter::ensure_whitespace_after(unsigned token_index, WhitespaceKind whitespace) {
    Token &token = tokens.tokens[token_index + 1];
    std::span<Token> attached_tokens = tokens.get_attached_tokens(token_index + 1);

    if (whitespace == WhitespaceKind::INDENT_ALLOW_EMPTY_LINE) {
        unsigned num_newlines = 0;

        if (attached_tokens[0].is(TKN_WHITESPACE)) {
            num_newlines = std::ranges::count(attached_tokens[0].value, '\n');
        }

        if (num_newlines >= 2) {
            whitespace = WhitespaceKind::INDENT_EMPTY_LINE;
        }
    }

    if (attached_tokens.empty()) {
        if (whitespace != WhitespaceKind::NONE) {
            add_insert_edit(token.position, whitespace_as_string(whitespace));
        }
    } else if (attached_tokens.size() == 1 && attached_tokens[0].is(TKN_WHITESPACE)) {
        if (whitespace == WhitespaceKind::NONE) {
            add_delete_edit(attached_tokens[0].range());
        } else {
            add_replace_edit(attached_tokens[0].range(), whitespace_as_string(whitespace));
        }
    } else {
        format_comments(attached_tokens, whitespace);
    }
}

void Formatter::format_comments(std::span<Token> &attached_tokens, WhitespaceKind whitespace) {
    // TODO: Remove empty lines before first node in parent

    for (unsigned i = 0; i < attached_tokens.size(); i++) {
        Token &attached_token = attached_tokens[i];
        bool is_last = i == attached_tokens.size() - 1;

        if (attached_token.is(TKN_WHITESPACE)) {
            TextRange range = attached_token.range();
            unsigned num_line_breaks = std::ranges::count(attached_token.value, '\n');

            if (num_line_breaks == 0 && i != attached_tokens.size() - 1) {
                if (attached_tokens[i + 1].is(TKN_COMMENT)) {
                    add_replace_edit(range, "  ");
                    continue;
                }
            }

            num_line_breaks = std::min(num_line_breaks, 2u);

            if (whitespace == WhitespaceKind::INDENT_OUT && is_last) {
                add_replace_edit(range, "\n" + build_indent(indentation - 1));
            } else {
                std::string line_breaks(num_line_breaks, '\n');
                add_replace_edit(range, line_breaks + build_indent(indentation));
            }
        } else if (attached_token.is(TKN_COMMENT)) {
            if (i == 0) {
                add_insert_edit(attached_token.position, "  ");
            }

            format_comment_text(attached_token);
        } else {
            ASSERT_UNREACHABLE;
        }
    }
}

void Formatter::format_comment_text(Token &comment_token) {
    std::string_view text = comment_token.value;

    if (text.size() >= 2 && text[1] != ' ') {
        add_insert_edit(comment_token.position + 1, " ");
    }

    std::optional<unsigned> trailing_whitespace_start;

    for (unsigned i = 0; i < text.size(); i++) {
        if (Utils::is_one_of(text[i], {' ', '\r', '\t'})) {
            if (!trailing_whitespace_start) {
                trailing_whitespace_start = i;
            }
        } else {
            trailing_whitespace_start = {};
        }
    }

    if (trailing_whitespace_start) {
        TextPosition range_start = comment_token.position + *trailing_whitespace_start;
        add_delete_edit({range_start, comment_token.end()});
    }
}

std::string Formatter::whitespace_as_string(WhitespaceKind whitespace) {
    switch (whitespace) {
        case WhitespaceKind::NONE: return "";
        case WhitespaceKind::SPACE: return " ";
        case WhitespaceKind::INDENT_ALLOW_EMPTY_LINE:
        case WhitespaceKind::INDENT: return "\n" + build_indent(indentation);
        case WhitespaceKind::INDENT_OUT: return "\n" + build_indent(indentation - 1);
        case WhitespaceKind::INDENT_EMPTY_LINE: return "\n\n" + build_indent(indentation);
    }
}

std::string Formatter::build_indent(unsigned indentation) {
    unsigned num_spaces = 4 * indentation;
    return std::string(num_spaces, ' ');
}

bool Formatter::followed_by_comment(unsigned token_index) {
    for (Token &attached_token : tokens.get_attached_tokens(token_index + 1)) {
        if (attached_token.is(TKN_COMMENT)) {
            return true;
        }
    }

    return false;
}

void Formatter::add_replace_edit(TextRange range, std::string replacement) {
    if (range.start != range.end) {
        if (file_content.substr(range.start, range.end - range.start) == replacement) {
            return;
        }
    }

    edits.push_back(Edit{.range = range, .replacement = std::move(replacement)});
}

void Formatter::add_insert_edit(TextPosition position, std::string replacement) {
    edits.push_back(Edit{.range{position, position}, .replacement = std::move(replacement)});
}

void Formatter::add_delete_edit(TextRange range) {
    edits.push_back(Edit{.range = range, .replacement = ""});
}

} // namespace banjo::lang
