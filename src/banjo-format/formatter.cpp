#include "formatter.hpp"

#include "banjo/ast/ast_node.hpp"
#include "banjo/lexer/lexer.hpp"
#include "banjo/parser/parser.hpp"
#include "banjo/utils/macros.hpp"
#include "banjo/utils/utils.hpp"

#include <fstream>
#include <initializer_list>

namespace banjo {
namespace format {

static const std::initializer_list<lang::ASTNodeType> NODES_WITH_BLOCKS{
    lang::ASTNodeType::AST_FUNCTION_DEFINITION,
    lang::ASTNodeType::AST_STRUCT_DEFINITION,
    lang::ASTNodeType::AST_GENERIC_STRUCT_DEFINITION,
    lang::ASTNodeType::AST_ENUM_DEFINITION,
    lang::ASTNodeType::AST_UNION,
    lang::ASTNodeType::AST_PROTO,
    lang::ASTNodeType::AST_BLOCK,
    lang::ASTNodeType::AST_IF_CHAIN,
    lang::ASTNodeType::AST_SWITCH,
    lang::ASTNodeType::AST_TRY,
    lang::ASTNodeType::AST_WHILE,
    lang::ASTNodeType::AST_FOR,
    lang::ASTNodeType::AST_META_FOR,
};

void Formatter::format(const std::filesystem::path &file_path) {
    std::ifstream stream(file_path);
    lang::Lexer lexer(stream, lang::Lexer::Mode::FORMATTING);
    std::vector<lang::Token> tokens = lexer.tokenize();

    lang::Parser parser(tokens, {}, lang::Parser::Mode::FORMATTING);
    lang::ParsedAST parse_result = parser.parse_module();

    if (!parse_result.is_valid) {
        return;
    }

    out_stream = std::ofstream(file_path);
    print_node(parse_result.module_);
}

void Formatter::print_node(lang::ASTNode *node) {
    switch (node->type) {
        case lang::ASTNodeType::AST_MODULE: print_mod(node); break;
        case lang::ASTNodeType::AST_FUNCTION_DEFINITION: print_func_def(node); break;
        case lang::ASTNodeType::AST_GENERIC_FUNCTION_DEFINITION: print_generic_func_def(node); break;
        case lang::ASTNodeType::AST_FUNC_DECL: print_func_decl(node); break;
        case lang::ASTNodeType::AST_NATIVE_FUNCTION_DECLARATION: print_native_func_decl(node); break;
        case lang::ASTNodeType::AST_CONSTANT: print_const(node); break;
        case lang::ASTNodeType::AST_STRUCT_DEFINITION: print_struct_def(node); break;
        case lang::ASTNodeType::AST_GENERIC_STRUCT_DEFINITION: print_generic_struct_def(node); break;
        case lang::ASTNodeType::AST_IMPL_LIST: break;
        case lang::ASTNodeType::AST_VAR: print_var(node); break;
        case lang::ASTNodeType::AST_IMPLICIT_TYPE_VAR: print_typeless_var(node); break;
        case lang::ASTNodeType::AST_NATIVE_VAR: print_native_var(node); break;
        case lang::ASTNodeType::AST_ENUM_DEFINITION: print_enum_def(node); break;
        case lang::ASTNodeType::AST_ENUM_VARIANT_LIST: break;
        case lang::ASTNodeType::AST_ENUM_VARIANT: break;
        case lang::ASTNodeType::AST_UNION: print_union_def(node); break;
        case lang::ASTNodeType::AST_UNION_CASE: print_union_case(node); break;
        case lang::ASTNodeType::AST_PROTO: print_proto_def(node); break;
        case lang::ASTNodeType::AST_TYPE_ALIAS: print_type_alias(node); break;
        case lang::ASTNodeType::AST_USE: print_use_decl(node); break;
        case lang::ASTNodeType::AST_USE_TREE_LIST: print_use_list(node); break;
        case lang::ASTNodeType::AST_USE_REBINDING: print_use_rebind(node); break;
        case lang::ASTNodeType::AST_ASSIGNMENT: print_assign_stmt(node); break;
        case lang::ASTNodeType::AST_ADD_ASSIGN: print_comp_assign_stmt(node, "+="); break;
        case lang::ASTNodeType::AST_SUB_ASSIGN: print_comp_assign_stmt(node, "-="); break;
        case lang::ASTNodeType::AST_MUL_ASSIGN: print_comp_assign_stmt(node, "*="); break;
        case lang::ASTNodeType::AST_DIV_ASSIGN: print_comp_assign_stmt(node, "/="); break;
        case lang::ASTNodeType::AST_MOD_ASSIGN: print_comp_assign_stmt(node, "%="); break;
        case lang::ASTNodeType::AST_BIT_AND_ASSIGN: print_comp_assign_stmt(node, "&="); break;
        case lang::ASTNodeType::AST_BIT_OR_ASSIGN: print_comp_assign_stmt(node, "|="); break;
        case lang::ASTNodeType::AST_BIT_XOR_ASSIGN: print_comp_assign_stmt(node, "^="); break;
        case lang::ASTNodeType::AST_SHL_ASSIGN: print_comp_assign_stmt(node, "<<="); break;
        case lang::ASTNodeType::AST_SHR_ASSIGN: print_comp_assign_stmt(node, ">>="); break;
        case lang::ASTNodeType::AST_FUNCTION_RETURN: print_return_stmt(node); break;
        case lang::ASTNodeType::AST_IF_CHAIN: print_if_stmt(node); break;
        case lang::ASTNodeType::AST_IF: break;
        case lang::ASTNodeType::AST_ELSE_IF: break;
        case lang::ASTNodeType::AST_ELSE: break;
        case lang::ASTNodeType::AST_SWITCH: print_switch_stmt(node); break;
        case lang::ASTNodeType::AST_SWITCH_CASE_LIST: break;
        case lang::ASTNodeType::AST_SWITCH_CASE: break;
        case lang::ASTNodeType::AST_SWITCH_DEFAULT_CASE: ASSERT_UNREACHABLE; break;
        case lang::ASTNodeType::AST_TRY: print_try_stmt(node); break;
        case lang::ASTNodeType::AST_TRY_SUCCESS_CASE: break;
        case lang::ASTNodeType::AST_TRY_ERROR_CASE: break;
        case lang::ASTNodeType::AST_TRY_ELSE_CASE: break;
        case lang::ASTNodeType::AST_WHILE: print_while_stmt(node); break;
        case lang::ASTNodeType::AST_FOR: print_for_stmt(node); break;
        case lang::ASTNodeType::AST_FOR_ITER_TYPE: break;
        case lang::ASTNodeType::AST_CONTINUE: emit("continue"); break;
        case lang::ASTNodeType::AST_BREAK: emit("break"); break;
        case lang::ASTNodeType::AST_BLOCK: print_block(node); break;
        case lang::ASTNodeType::AST_INT_LITERAL: emit(node->value); break;
        case lang::ASTNodeType::AST_FLOAT_LITERAL: emit(node->value); break;
        case lang::ASTNodeType::AST_FALSE: emit("false"); break;
        case lang::ASTNodeType::AST_TRUE: emit("true"); break;
        case lang::ASTNodeType::AST_CHAR_LITERAL: print_char_literal(node); break;
        case lang::ASTNodeType::AST_NULL: emit("null"); break;
        case lang::ASTNodeType::AST_NONE: emit("none"); break;
        case lang::ASTNodeType::AST_UNDEFINED: emit("undefined"); break;
        case lang::ASTNodeType::AST_ARRAY_EXPR: print_array_expr(node); break;
        case lang::ASTNodeType::AST_STRING_LITERAL: print_string_literal(node); break;
        case lang::ASTNodeType::AST_STRUCT_INSTANTIATION: print_struct_literal(node); break;
        case lang::ASTNodeType::AST_ANON_STRUCT_LITERAL: print_anon_struct_literal(node); break;
        case lang::ASTNodeType::AST_STRUCT_FIELD_VALUE_LIST: break;
        case lang::ASTNodeType::AST_STRUCT_FIELD_VALUE: print_struct_literal_entry(node); break;
        case lang::ASTNodeType::AST_MAP_EXPR: print_map_expr(node); break;
        case lang::ASTNodeType::AST_MAP_EXPR_PAIR: print_map_literal_entry(node); break;
        case lang::ASTNodeType::AST_CLOSURE: print_closure_literal(node); break;
        case lang::ASTNodeType::AST_PAREN_EXPR: print_paren_expr(node); break;
        case lang::ASTNodeType::AST_TUPLE_EXPR: print_tuple_expr(node); break;
        case lang::ASTNodeType::AST_IDENTIFIER: emit(node->value); break;
        case lang::ASTNodeType::AST_SELF: emit("self"); break;
        case lang::ASTNodeType::AST_OPERATOR_ADD: print_binary_expr(node, "+"); break;
        case lang::ASTNodeType::AST_OPERATOR_SUB: print_binary_expr(node, "-"); break;
        case lang::ASTNodeType::AST_OPERATOR_MUL: print_binary_expr(node, "*"); break;
        case lang::ASTNodeType::AST_OPERATOR_DIV: print_binary_expr(node, "/"); break;
        case lang::ASTNodeType::AST_OPERATOR_MOD: print_binary_expr(node, "%"); break;
        case lang::ASTNodeType::AST_OPERATOR_BIT_AND: print_binary_expr(node, "&"); break;
        case lang::ASTNodeType::AST_OPERATOR_BIT_OR: print_binary_expr(node, "|"); break;
        case lang::ASTNodeType::AST_OPERATOR_BIT_XOR: print_binary_expr(node, "^"); break;
        case lang::ASTNodeType::AST_OPERATOR_SHL: print_binary_expr(node, "<<"); break;
        case lang::ASTNodeType::AST_OPERATOR_SHR: print_binary_expr(node, ">>"); break;
        case lang::ASTNodeType::AST_OPERATOR_EQ: print_binary_expr(node, "=="); break;
        case lang::ASTNodeType::AST_OPERATOR_NE: print_binary_expr(node, "!="); break;
        case lang::ASTNodeType::AST_OPERATOR_GT: print_binary_expr(node, ">"); break;
        case lang::ASTNodeType::AST_OPERATOR_LT: print_binary_expr(node, "<"); break;
        case lang::ASTNodeType::AST_OPERATOR_GE: print_binary_expr(node, ">="); break;
        case lang::ASTNodeType::AST_OPERATOR_LE: print_binary_expr(node, "<="); break;
        case lang::ASTNodeType::AST_OPERATOR_AND: print_binary_expr(node, "&&"); break;
        case lang::ASTNodeType::AST_OPERATOR_OR: print_binary_expr(node, "||"); break;
        case lang::ASTNodeType::AST_OPERATOR_NEG: print_unary_expr(node, "-"); break;
        case lang::ASTNodeType::AST_OPERATOR_BIT_NOT: print_unary_expr(node, "~"); break;
        case lang::ASTNodeType::AST_OPERATOR_REF: print_unary_expr(node, "&"); break;
        case lang::ASTNodeType::AST_STAR_EXPR: print_unary_expr(node, "*"); break;
        case lang::ASTNodeType::AST_OPERATOR_NOT: print_unary_expr(node, "!"); break;
        case lang::ASTNodeType::AST_CAST: print_cast_expr(node); break;
        case lang::ASTNodeType::AST_FUNCTION_CALL: print_call_expr(node); break;
        case lang::ASTNodeType::AST_FUNCTION_ARGUMENT_LIST: break;
        case lang::ASTNodeType::AST_DOT_OPERATOR: print_dot_expr(node); break;
        case lang::ASTNodeType::AST_RANGE: print_range_expr(node); break;
        case lang::ASTNodeType::AST_ARRAY_ACCESS: print_bracket_expr(node); break;
        case lang::ASTNodeType::AST_I8: emit("i8"); break;
        case lang::ASTNodeType::AST_I16: emit("i16"); break;
        case lang::ASTNodeType::AST_I32: emit("i32"); break;
        case lang::ASTNodeType::AST_I64: emit("i64"); break;
        case lang::ASTNodeType::AST_U8: emit("u8"); break;
        case lang::ASTNodeType::AST_U16: emit("u16"); break;
        case lang::ASTNodeType::AST_U32: emit("u32"); break;
        case lang::ASTNodeType::AST_U64: emit("u64"); break;
        case lang::ASTNodeType::AST_F32: emit("f32"); break;
        case lang::ASTNodeType::AST_F64: emit("f64"); break;
        case lang::ASTNodeType::AST_USIZE: emit("usize"); break;
        case lang::ASTNodeType::AST_BOOL: emit("bool"); break;
        case lang::ASTNodeType::AST_ADDR: emit("addr"); break;
        case lang::ASTNodeType::AST_VOID: emit("void"); break;
        case lang::ASTNodeType::AST_STATIC_ARRAY_TYPE: print_static_array_type(node); break;
        case lang::ASTNodeType::AST_FUNCTION_DATA_TYPE: print_func_type(node); break;
        case lang::ASTNodeType::AST_OPTIONAL_DATA_TYPE: print_optional_type(node); break;
        case lang::ASTNodeType::AST_RESULT_TYPE: print_result_type(node); break;
        case lang::ASTNodeType::AST_CLOSURE_TYPE: print_closure_type(node); break;
        case lang::ASTNodeType::AST_EXPLICIT_TYPE: print_explicit_type(node); break;
        case lang::ASTNodeType::AST_PARAM_SEQUENCE_TYPE: emit("..."); break;
        case lang::ASTNodeType::AST_META_EXPR: print_meta_access(node); break;
        case lang::ASTNodeType::AST_META_FIELD_ACCESS: print_dot_expr(node); break;
        case lang::ASTNodeType::AST_META_METHOD_CALL: print_call_expr(node); break;
        case lang::ASTNodeType::AST_META_IF: print_meta_if_stmt(node); break;
        case lang::ASTNodeType::AST_META_IF_CONDITION: break;
        case lang::ASTNodeType::AST_META_ELSE: break;
        case lang::ASTNodeType::AST_META_FOR: print_meta_for_stmt(node); break;
        case lang::ASTNodeType::AST_PARAM_LIST: break;
        case lang::ASTNodeType::AST_PARAM: print_param(node); break;
        case lang::ASTNodeType::AST_QUALIFIER_LIST: print_qualifier_list(node); break;
        case lang::ASTNodeType::AST_QUALIFIER: break;
        case lang::ASTNodeType::AST_GENERIC_PARAM_LIST: break;
        case lang::ASTNodeType::AST_GENERIC_PARAMETER: print_generic_param(node); break;
        case lang::ASTNodeType::AST_EMPTY_LINE: break;
        case lang::ASTNodeType::AST_ERROR: break;
        case lang::ASTNodeType::AST_COMPLETION_TOKEN: break;
        case lang::ASTNodeType::AST_INVALID: break;
    }
}

void Formatter::print_mod(lang::ASTNode *node) {
    lang::ASTNode *block_node = node->first_child;

    print_block_children(block_node);
}

void Formatter::print_func_def(lang::ASTNode *node) {
    lang::ASTNode *qualifiers_node = node->first_child;
    lang::ASTNode *name_node = qualifiers_node->next_sibling;
    lang::ASTNode *params_node = name_node->next_sibling;
    lang::ASTNode *return_type_node = params_node->next_sibling;
    lang::ASTNode *block_node = return_type_node->next_sibling;

    print_qualifier_list(qualifiers_node);
    emit("func ");
    print_node(name_node);
    print_func_type(params_node, return_type_node);
    emit(" ");
    print_block_of_decl(block_node);
}

void Formatter::print_generic_func_def(lang::ASTNode *node) {
    lang::ASTNode *qualifiers_node = node->first_child;
    lang::ASTNode *name_node = qualifiers_node->next_sibling;
    lang::ASTNode *generic_params_node = name_node->next_sibling;
    lang::ASTNode *params_node = generic_params_node->next_sibling;
    lang::ASTNode *return_type_node = params_node->next_sibling;
    lang::ASTNode *block_node = return_type_node->next_sibling;

    print_qualifier_list(qualifiers_node);
    emit("func ");
    print_node(name_node);
    print_generic_param_list(generic_params_node);
    print_func_type(params_node, return_type_node);
    emit(" ");
    print_block_of_decl(block_node);
}

void Formatter::print_func_decl(lang::ASTNode *node) {
    lang::ASTNode *qualifiers_node = node->first_child;
    lang::ASTNode *name_node = qualifiers_node->next_sibling;
    lang::ASTNode *params_node = name_node->next_sibling;
    lang::ASTNode *return_type_node = params_node->next_sibling;

    print_qualifier_list(qualifiers_node);
    emit("func ");
    print_node(name_node);
    print_func_type(params_node, return_type_node);
}

void Formatter::print_native_func_decl(lang::ASTNode *node) {
    emit("native ");
    print_func_decl(node);
}

void Formatter::print_const(lang::ASTNode *node) {
    lang::ASTNode *name_node = node->first_child;
    lang::ASTNode *type_node = name_node->next_sibling;
    lang::ASTNode *value_node = type_node->next_sibling;

    emit("const ");
    print_node(name_node);
    emit(": ");
    print_node(type_node);
    emit(" = ");
    print_node(value_node);
}

void Formatter::print_struct_def(lang::ASTNode *node) {
    lang::ASTNode *name_node = node->first_child;
    lang::ASTNode *impls_node = name_node->next_sibling;
    lang::ASTNode *block_node = impls_node->next_sibling;

    emit("struct ");
    print_node(name_node);

    if (impls_node->has_children()) {
        emit(": ");

        for (lang::ASTNode *child = impls_node->first_child; child; child = child->next_sibling) {
            print_node(child);

            if (child->next_sibling) {
                emit(", ");
            }
        }
    }

    emit(" ");
    print_block_of_decl(block_node);
}

void Formatter::print_generic_struct_def(lang::ASTNode *node) {
    lang::ASTNode *name_node = node->first_child;
    lang::ASTNode *generic_params_node = name_node->next_sibling;
    lang::ASTNode *block_node = generic_params_node->next_sibling;

    emit("struct ");
    print_node(name_node);
    print_generic_param_list(generic_params_node);
    emit(" ");
    print_block_of_decl(block_node);
}

void Formatter::print_var(lang::ASTNode *node) {
    lang::ASTNode *name_node = node->first_child;
    lang::ASTNode *type_node = name_node->next_sibling;
    lang::ASTNode *value_node = type_node->next_sibling;

    emit("var ");
    print_node(name_node);
    emit(": ");
    print_node(type_node);

    if (value_node) {
        emit(" = ");
        print_node(value_node);
    }
}

void Formatter::print_typeless_var(lang::ASTNode *node) {
    lang::ASTNode *name_node = node->first_child;
    lang::ASTNode *value_node = name_node->next_sibling;

    emit("var ");
    print_node(name_node);
    emit(" = ");
    print_node(value_node);
}

void Formatter::print_native_var(lang::ASTNode *node) {
    lang::ASTNode *name_node = node->first_child;
    lang::ASTNode *type_node = name_node->next_sibling;

    emit("native var ");
    print_node(name_node);
    emit(": ");
    print_node(type_node);
}

void Formatter::print_enum_def(lang::ASTNode *node) {
    lang::ASTNode *name_node = node->first_child;
    lang::ASTNode *variants_node = name_node->next_sibling;

    emit("enum ");
    print_node(name_node);

    emit(" {");

    if (variants_node->has_children()) {
        indent += 1;
        emit("\n");

        for (lang::ASTNode *variant_node = variants_node->first_child; variant_node;
             variant_node = variant_node->next_sibling) {
            indent_line();
            print_enum_variant(variant_node);
            emit(",\n");
        }

        indent -= 1;
    }

    emit("}");
}

void Formatter::print_enum_variant(lang::ASTNode *node) {
    lang::ASTNode *name_node = node->first_child;
    lang::ASTNode *value_node = name_node->next_sibling;

    print_node(name_node);

    if (value_node) {
        emit(" = ");
        print_node(value_node);
    }
}

void Formatter::print_union_def(lang::ASTNode *node) {
    lang::ASTNode *name_node = node->first_child;
    lang::ASTNode *block_node = name_node->next_sibling;

    emit("union ");
    print_node(name_node);
    emit(" ");
    print_block_of_decl(block_node);
}

void Formatter::print_union_case(lang::ASTNode *node) {
    lang::ASTNode *name_node = node->first_child;
    lang::ASTNode *fields_node = name_node->next_sibling;

    emit("case ");
    print_node(name_node);
    emit("(");
    print_list(fields_node);
    emit(")");
}

void Formatter::print_proto_def(lang::ASTNode *node) {
    lang::ASTNode *name_node = node->first_child;
    lang::ASTNode *block_node = name_node->next_sibling;

    emit("proto ");
    print_node(name_node);
    emit(" ");
    print_block_of_decl(block_node);
}

void Formatter::print_type_alias(lang::ASTNode *node) {
    lang::ASTNode *name_node = node->first_child;
    lang::ASTNode *underlying_type_node = name_node->next_sibling;

    emit("type ");
    print_node(name_node);
    emit(" = ");
    print_node(underlying_type_node);
}

void Formatter::print_use_decl(lang::ASTNode *node) {
    lang::ASTNode *root_item_node = node->first_child;

    emit("use ");
    print_node(root_item_node);
}

void Formatter::print_use_rebind(lang::ASTNode *node) {
    lang::ASTNode *target_name_node = node->first_child;
    lang::ASTNode *local_name_node = target_name_node->next_sibling;

    print_node(target_name_node);
    emit(" as ");
    print_node(local_name_node);
}

void Formatter::print_use_list(lang::ASTNode *node) {
    emit("{");
    print_list(node);
    emit("}");
}

void Formatter::print_block(lang::ASTNode *node) {
    emit("{");

    if (node->has_children()) {
        indent += 1;

        emit("\n");
        print_block_children(node);

        indent -= 1;
        indent_line();
    }

    emit("}");
}

void Formatter::print_block_of_decl(lang::ASTNode *node) {
    print_block(node);

    if (node->has_children()) {
        insert_empty_line = true;
    }
}

void Formatter::print_assign_stmt(lang::ASTNode *node) {
    lang::ASTNode *lhs_node = node->first_child;
    lang::ASTNode *rhs_node = lhs_node->next_sibling;

    print_node(lhs_node);
    emit(" = ");
    print_node(rhs_node);
}

void Formatter::print_comp_assign_stmt(lang::ASTNode *node, std::string_view op_text) {
    lang::ASTNode *lhs_node = node->first_child;
    lang::ASTNode *rhs_node = lhs_node->next_sibling;

    print_node(lhs_node);
    emit(" ");
    emit(op_text);
    emit(" ");
    print_node(rhs_node);
}

void Formatter::print_return_stmt(lang::ASTNode *node) {
    lang::ASTNode *value_node = node->first_child;

    emit("return");

    if (value_node) {
        emit(" ");
        print_node(value_node);
    }
}

void Formatter::print_if_stmt(lang::ASTNode *node) {
    for (lang::ASTNode *child = node->first_child; child; child = child->next_sibling) {
        if (child->type == lang::ASTNodeType::AST_IF) {
            print_if_condition(child, true);
        } else if (child->type == lang::ASTNodeType::AST_ELSE_IF) {
            print_if_condition(child, false);
        } else if (child->type == lang::ASTNodeType::AST_ELSE) {
            print_else(child);
        } else {
            ASSERT_UNREACHABLE;
        }
    }
}

void Formatter::print_if_condition(lang::ASTNode *node, bool first) {
    lang::ASTNode *condition_node = node->first_child;
    lang::ASTNode *block_node = condition_node->next_sibling;

    emit(first ? "if " : " else if ");
    print_node(condition_node);
    emit(' ');
    print_block(block_node);
}

void Formatter::print_else(lang::ASTNode *node) {
    lang::ASTNode *block_node = node->first_child;

    emit(" else ");
    print_block(block_node);
}

void Formatter::print_switch_stmt(lang::ASTNode *node) {
    lang::ASTNode *value_node = node->first_child;
    lang::ASTNode *cases_node = value_node->next_sibling;

    emit("switch ");
    print_node(value_node);
    emit(" {");
    indent += 1;

    if (cases_node->has_children()) {
        emit("\n");

        for (lang::ASTNode *child = cases_node->first_child; child; child = child->next_sibling) {
            indent_line();
            print_switch_case(child);
            emit("\n");
        }
    }

    indent -= 1;
    indent_line();
    emit("}");
}

void Formatter::print_switch_case(lang::ASTNode *node) {
    lang::ASTNode *name_node = node->first_child;
    lang::ASTNode *type_node = name_node->next_sibling;
    lang::ASTNode *block_node = type_node->next_sibling;

    emit("case ");
    print_node(name_node);
    emit(": ");
    print_node(type_node);
    emit(" ");
    print_block(block_node);
}

void Formatter::print_try_stmt(lang::ASTNode *node) {
    for (lang::ASTNode *child = node->first_child; child; child = child->next_sibling) {
        if (child->type == lang::ASTNodeType::AST_TRY_SUCCESS_CASE) {
            print_try_success_case(child);
        } else if (child->type == lang::ASTNodeType::AST_TRY_ERROR_CASE) {
            print_try_error_case(child);
        } else if (child->type == lang::ASTNodeType::AST_TRY_ELSE_CASE) {
            print_try_else_case(child);
        } else {
            ASSERT_UNREACHABLE;
        }
    }
}

void Formatter::print_try_success_case(lang::ASTNode *node) {
    lang::ASTNode *name_node = node->first_child;
    lang::ASTNode *expr_node = name_node->next_sibling;
    lang::ASTNode *block_node = expr_node->next_sibling;

    emit("try ");
    print_node(name_node);
    emit(" in ");
    print_node(expr_node);
    emit(" ");
    print_block(block_node);
}

void Formatter::print_try_error_case(lang::ASTNode *node) {
    lang::ASTNode *name_node = node->first_child;
    lang::ASTNode *type_node = name_node->next_sibling;
    lang::ASTNode *block_node = type_node->next_sibling;

    emit(" except ");
    print_node(name_node);
    emit(": ");
    print_node(type_node);
    emit(" ");
    print_block(block_node);
}

void Formatter::print_try_else_case(lang::ASTNode *node) {
    lang::ASTNode *block_node = node->first_child;

    emit(" else ");
    print_block(block_node);
}

void Formatter::print_while_stmt(lang::ASTNode *node) {
    lang::ASTNode *condition_node = node->first_child;
    lang::ASTNode *block_node = condition_node->next_sibling;

    emit("while ");
    print_node(condition_node);
    emit(' ');
    print_block(block_node);
}

void Formatter::print_for_stmt(lang::ASTNode *node) {
    lang::ASTNode *iter_type_node = node->first_child;
    lang::ASTNode *name_node = iter_type_node->next_sibling;
    lang::ASTNode *range_node = name_node->next_sibling;
    lang::ASTNode *block_node = range_node->next_sibling;

    emit("for ");

    if (iter_type_node->value == "*") {
        emit("*");
    }

    emit(name_node->value);
    emit(" in ");
    print_node(range_node);
    emit(' ');
    print_block(block_node);
}

void Formatter::print_char_literal(lang::ASTNode *node) {
    emit('\'');
    emit(node->value);
    emit('\'');
}

void Formatter::print_array_expr(lang::ASTNode *node) {
    emit("[");
    print_list(node);
    emit("]");
}

void Formatter::print_string_literal(lang::ASTNode *node) {
    emit('\"');

    for (char c : node->value) {
        // FIXME: This whole escape character thing is quite broken.
        if (c == '\"') {
            emit("\\\"");
        } else {
            emit(c);
        }
    }

    emit('\"');
}

void Formatter::print_struct_literal(lang::ASTNode *node) {
    lang::ASTNode *name_node = node->first_child;
    lang::ASTNode *entries_node = name_node->next_sibling;

    print_node(name_node);
    emit(" ");
    print_struct_literal_entries(entries_node);
}

void Formatter::print_anon_struct_literal(lang::ASTNode *node) {
    lang::ASTNode *entries_node = node->first_child;
    print_struct_literal_entries(entries_node);
}

void Formatter::print_struct_literal_entries(lang::ASTNode *node) {
    emit(node->flags.trailing_comma ? "{" : "{ ");
    print_list(node);
    emit(node->flags.trailing_comma ? "}" : " }");
}

void Formatter::print_struct_literal_entry(lang::ASTNode *node) {
    lang::ASTNode *name_node = node->first_child;
    lang::ASTNode *value_node = name_node->next_sibling;

    if (value_node->type == lang::ASTNodeType::AST_IDENTIFIER && value_node->value == name_node->value) {
        emit(name_node->value);
        return;
    }

    print_node(name_node);
    emit(": ");
    print_node(value_node);
}

void Formatter::print_map_expr(lang::ASTNode *node) {
    emit("[");
    print_list(node);
    emit("]");
}

void Formatter::print_map_literal_entry(lang::ASTNode *node) {
    lang::ASTNode *key_node = node->first_child;
    lang::ASTNode *value_node = key_node->next_sibling;

    print_node(key_node);
    emit(": ");
    print_node(value_node);
}

void Formatter::print_closure_literal(lang::ASTNode *node) {
    lang::ASTNode *params_node = node->first_child;
    lang::ASTNode *return_type_node = params_node->next_sibling;
    lang::ASTNode *block_node = return_type_node->next_sibling;

    print_closure_type(params_node, return_type_node);
    emit(" ");
    print_block(block_node);
}

void Formatter::print_paren_expr(lang::ASTNode *node) {
    lang::ASTNode *expr_node = node->first_child;

    emit("(");
    print_node(expr_node);
    emit(")");
}

void Formatter::print_tuple_expr(lang::ASTNode *node) {
    emit("(");
    print_list(node);
    emit(")");
}

void Formatter::print_binary_expr(lang::ASTNode *node, std::string_view op_text) {
    lang::ASTNode *lhs_node = node->first_child;
    lang::ASTNode *rhs_node = lhs_node->next_sibling;

    print_node(lhs_node);
    emit(" ");
    emit(op_text);
    emit(" ");
    print_node(rhs_node);
}

void Formatter::print_unary_expr(lang::ASTNode *node, std::string_view op_text) {
    lang::ASTNode *value_node = node->first_child;

    emit(op_text);
    print_node(value_node);
}

void Formatter::print_cast_expr(lang::ASTNode *node) {
    lang::ASTNode *value_node = node->first_child;
    lang::ASTNode *type_node = value_node->next_sibling;

    print_node(value_node);
    emit(" as ");
    print_node(type_node);
}

void Formatter::print_call_expr(lang::ASTNode *node) {
    lang::ASTNode *callee_node = node->first_child;
    lang::ASTNode *args_node = callee_node->next_sibling;

    print_node(callee_node);
    emit("(");
    print_list(args_node);
    emit(")");
}

void Formatter::print_dot_expr(lang::ASTNode *node) {
    lang::ASTNode *lhs_node = node->first_child;
    lang::ASTNode *rhs_node = lhs_node->next_sibling;

    print_node(lhs_node);
    emit(".");
    print_node(rhs_node);
}

void Formatter::print_range_expr(lang::ASTNode *node) {
    lang::ASTNode *lhs_node = node->first_child;
    lang::ASTNode *rhs_node = lhs_node->next_sibling;

    print_node(lhs_node);
    emit("..");
    print_node(rhs_node);
}

void Formatter::print_bracket_expr(lang::ASTNode *node) {
    lang::ASTNode *lhs_node = node->first_child;
    lang::ASTNode *rhs_node = lhs_node->next_sibling;

    print_node(lhs_node);
    emit("[");
    print_list(rhs_node);
    emit("]");
}

void Formatter::print_static_array_type(lang::ASTNode *node) {
    lang::ASTNode *base_type_node = node->first_child;
    lang::ASTNode *length_node = base_type_node->next_sibling;

    emit("[");
    print_node(base_type_node);
    emit("; ");
    print_node(length_node);
    emit("]");
}

void Formatter::print_func_type(lang::ASTNode *node) {
    lang::ASTNode *params_node = node->first_child;
    lang::ASTNode *return_type_node = params_node->next_sibling;

    emit("func");
    print_func_type(params_node, return_type_node);
}

void Formatter::print_optional_type(lang::ASTNode *node) {
    lang::ASTNode *base_type_node = node->first_child;

    emit("?");
    print_node(base_type_node);
}

void Formatter::print_result_type(lang::ASTNode *node) {
    lang::ASTNode *value_type_node = node->first_child;
    lang::ASTNode *error_type_node = value_type_node->next_sibling;

    print_node(value_type_node);
    emit(" except ");
    print_node(error_type_node);
}

void Formatter::print_closure_type(lang::ASTNode *node) {
    lang::ASTNode *params_node = node->first_child;
    lang::ASTNode *return_type_node = params_node->next_sibling;

    print_closure_type(params_node, return_type_node);
}

void Formatter::print_explicit_type(lang::ASTNode *node) {
    lang::ASTNode *type_node = node->first_child;

    emit("type(");
    print_node(type_node);
    emit(")");
}

void Formatter::print_meta_access(lang::ASTNode *node) {
    lang::ASTNode *expr_node = node->first_child;

    emit("meta(");
    print_node(expr_node);
    emit(")");
}

void Formatter::print_meta_if_stmt(lang::ASTNode *node) {
    emit("meta ");

    bool first = true;

    for (lang::ASTNode *child = node->first_child; child; child = child->next_sibling) {
        if (child->type == lang::ASTNodeType::AST_META_IF_CONDITION) {
            print_if_condition(child, first);
            first = false;
        } else if (child->type == lang::ASTNodeType::AST_META_ELSE) {
            print_else(child);
        } else {
            ASSERT_UNREACHABLE;
        }
    }
}

void Formatter::print_meta_for_stmt(lang::ASTNode *node) {
    lang::ASTNode *name_node = node->first_child;
    lang::ASTNode *range_node = name_node->next_sibling;
    lang::ASTNode *block_node = range_node->next_sibling;

    emit("meta for ");
    emit(name_node->value);
    emit(" in ");
    print_node(range_node);
    emit(' ');
    print_block(block_node);
}

void Formatter::print_func_type(lang::ASTNode *params_node, lang::ASTNode *return_type_node) {
    emit("(");
    print_list(params_node);
    emit(")");

    if (return_type_node->type != lang::ASTNodeType::AST_VOID) {
        emit(" -> ");
        print_node(return_type_node);
    }
}

void Formatter::print_closure_type(lang::ASTNode *params_node, lang::ASTNode *return_type_node) {
    emit("|");
    print_list(params_node);
    emit("|");

    if (return_type_node->type != lang::ASTNodeType::AST_VOID) {
        emit(" -> ");
        print_node(return_type_node);
    }
}

void Formatter::print_param(lang::ASTNode *node) {
    lang::ASTNode *name_node = node->first_child;

    if (name_node->type == lang::AST_IDENTIFIER) {
        lang::ASTNode *type_node = name_node->next_sibling;

        emit(name_node->value);
        emit(": ");
        print_node(type_node);
    } else if (name_node->type == lang::AST_SELF) {
        emit("self");
    } else {
        ASSERT_UNREACHABLE;
    }
}

void Formatter::print_qualifier_list(lang::ASTNode *node) {
    for (lang::ASTNode *child = node->first_child; child; child = child->next_sibling) {
        emit(child->value);
        emit(" ");
    }
}

void Formatter::print_generic_param_list(lang::ASTNode *node) {
    emit("[");
    print_list(node);
    emit("]");
}

void Formatter::print_generic_param(lang::ASTNode *node) {
    lang::ASTNode *name_node = node->first_child;
    lang::ASTNode *type_node = name_node->next_sibling;

    print_node(name_node);

    if (type_node) {
        emit(": ");
        print_node(type_node);
    }
}

void Formatter::print_list(lang::ASTNode *node) {
    if (!node->flags.trailing_comma) {
        for (lang::ASTNode *child = node->first_child; child; child = child->next_sibling) {
            print_node(child);

            if (child->next_sibling) {
                emit(", ");
            }
        }
    } else {
        emit("\n");
        indent += 1;

        for (lang::ASTNode *child = node->first_child; child; child = child->next_sibling) {
            indent_line();
            print_node(child);
            emit(",\n");
        }

        indent -= 1;
        indent_line();
    }
}

void Formatter::print_block_children(lang::ASTNode *node) {
    for (lang::ASTNode *child = node->first_child; child; child = child->next_sibling) {
        try_insert_empty_line(child);

        if (child->type == lang::ASTNodeType::AST_EMPTY_LINE) {
            continue;
        }

        indent_line();
        print_node(child);

        if (!Utils::is_one_of(child->type, NODES_WITH_BLOCKS)) {
            emit(";");
        }

        emit("\n");
    }
}

void Formatter::indent_line() {
    for (unsigned i = 0; i < indent; i++) {
        emit("    ");
    }
}

void Formatter::try_insert_empty_line(lang::ASTNode *child) {
    if (insert_empty_line || child->type == lang::ASTNodeType::AST_EMPTY_LINE) {
        emit("\n");
        insert_empty_line = false;
    }
}

void Formatter::emit(std::string_view text) {
    out_stream << text;
}

void Formatter::emit(char c) {
    out_stream << c;
}

} // namespace format
} // namespace banjo
