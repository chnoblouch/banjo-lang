#include "ast_writer.hpp"

#include "banjo/ast/ast_node.hpp"

namespace banjo {

namespace lang {

ASTWriter::ASTWriter(std::ostream &stream) : stream(stream) {}

void ASTWriter::write(ASTNode *node) {
    write(node, 0);
}

void ASTWriter::write_all(ModuleList &module_list) {
    for (const std::unique_ptr<SourceFile> &file : module_list) {
        write(file->ast_mod.get());
    }
}

void ASTWriter::write(ASTNode *node, unsigned indentation) {
    for (int i = 0; i < indentation; i++) {
        stream << "  ";
    }

    ASTNodeType type = node->type;
    unsigned range_start = node->range.start;
    unsigned range_end = node->range.end;

    stream << get_type_name(type) << " [" << range_start << ":" << range_end << "]";

    if (node->value.length() > 0) {
        stream << ": " << node->value;
    }

    stream << "\n";

    for (ASTNode *child = node->first_child; child; child = child->next_sibling) {
        write(child, indentation + 1);
    }
}

const char *ASTWriter::get_type_name(ASTNodeType type) {
    switch (type) {
        case AST_MODULE: return "MODULE";
        case AST_FUNC_DEF: return "FUNC_DEF";
        case AST_GENERIC_FUNC_DEF: return "GENERIC_FUNC_DEF";
        case AST_FUNC_DECL: return "FUNC_DECL";
        case AST_NATIVE_FUNC_DECL: return "NATIVE_FUNC_DECL";
        case AST_CONST_DEF: return "CONST_DEF";
        case AST_STRUCT_DEF: return "STRUCT_DEF";
        case AST_GENERIC_STRUCT_DEF: return "GENERIC_STRUCT_DEF";
        case AST_VAR_DEF: return "VAR_DEF";
        case AST_TYPELESS_VAR_DEF: return "TYPELESS_VAR_DEF";
        case AST_REF_VAR_DEF: return "REF_VAR_DEF";
        case AST_TYPELESS_REF_VAR_DEF: return "TYPELESS_REF_VAR_DEF";
        case AST_REF_MUT_VAR_DEF: return "REF_MUT_VAR_DEF";
        case AST_TYPELESS_REF_MUT_VAR_DEF: return "TYPELESS_REF_MUT_VAR_DEF";
        case AST_NATIVE_VAR_DECL: return "NATIVE_VAR_DECL";
        case AST_ENUM_DEF: return "ENUM_DEF";
        case AST_ENUM_VARIANT_LIST: return "ENUM_VARIANT_LIST";
        case AST_ENUM_VARIANT: return "ENUM_VARIANT";
        case AST_UNION_DEF: return "UNION_DEF";
        case AST_UNION_CASE: return "UNION_CASE";
        case AST_PROTO_DEF: return "PROTO_DEF";
        case AST_IMPL_LIST: return "IMPL_LIST";
        case AST_TYPE_ALIAS_DEF: return "TYPE_ALIAS_DEF";
        case AST_USE_DECL: return "USE_DECL";
        case AST_USE_LIST: return "USE_LIST";
        case AST_USE_REBIND: return "USE_REBIND";
        case AST_EXPR_STMT: return "EXPR_STMT";
        case AST_ASSIGN_STMT: return "ASSIGN_STMT";
        case AST_ADD_ASSIGN_STMT: return "ADD_ASSIGN_STMT";
        case AST_SUB_ASSIGN_STMT: return "SUB_ASSIGN_STMT";
        case AST_MUL_ASSIGN_STMT: return "MUL_ASSIGN_STMT";
        case AST_DIV_ASSIGN_STMT: return "DIV_ASSIGN_STMT";
        case AST_MOD_ASSIGN_STMT: return "MOD_ASSIGN_STMT";
        case AST_BIT_AND_ASSIGN_STMT: return "BIT_AND_ASSIGN_STMT";
        case AST_BIT_OR_ASSIGN_STMT: return "BIT_OR_ASSIGN_STMT";
        case AST_BIT_XOR_ASSIGN_STMT: return "BIT_XOR_ASSIGN_STMT";
        case AST_SHL_ASSIGN_STMT: return "SHL_ASSIGN_STMT";
        case AST_SHR_ASSIGN_STMT: return "SHR_ASSIGN_STMT";
        case AST_RETURN_STMT: return "RETURN_STMT";
        case AST_IF_STMT: return "IF_STMT";
        case AST_IF_BRANCH: return "IF_BRANCH";
        case AST_ELSE_IF_BRANCH: return "ELSE_IF_BRANCH";
        case AST_ELSE_BRANCH: return "ELSE_BRANCH";
        case AST_SWITCH_STMT: return "SWITCH_STMT";
        case AST_SWITCH_CASE_LIST: return "SWITCH_CASE_LIST";
        case AST_SWITCH_CASE_BRANCH: return "SWITCH_CASE_BRANCH";
        case AST_SWITCH_DEFAULT_BRANCH: return "SWITCH_DEFAULT_BRANCH";
        case AST_TRY_STMT: return "TRY_STMT";
        case AST_TRY_SUCCESS_BRANCH: return "TRY_SUCCESS_BRANCH";
        case AST_TRY_EXCEPT_BRANCH: return "TRY_EXCEPT_BRANCH";
        case AST_TRY_ELSE_BRANCH: return "TRY_ELSE_BRANCH";
        case AST_WHILE_STMT: return "WHILE_STMT";
        case AST_FOR_STMT: return "FOR_STMT";
        case AST_FOR_REF_STMT: return "FOR_REF_STMT";
        case AST_FOR_REF_MUT_STMT: return "FOR_REF_MUT_STMT";
        case AST_CONTINUE_STMT: return "CONTINUE_STMT";
        case AST_BREAK_STMT: return "BREAK_STMT";
        case AST_META_IF_STMT: return "META_IF_STMT";
        case AST_META_IF_BRANCH: return "META_IF_BRANCH";
        case AST_META_ELSE_IF_BRANCH: return "META_ELSE_IF_BRANCH";
        case AST_META_ELSE_BRANCH: return "META_ELSE_BRANCH";
        case AST_META_FOR_STMT: return "META_FOR_STMT";
        case AST_INT_LITERAL: return "INT_LITERAL";
        case AST_FP_LITERAL: return "FP_LITERAL";
        case AST_FALSE_LITERAL: return "FALSE_LITERAL";
        case AST_TRUE_LITERAL: return "TRUE_LITERAL";
        case AST_CHAR_LITERAL: return "CHAR_LITERAL";
        case AST_NULL_LITERAL: return "NULL_LITERAL";
        case AST_NONE_LITERAL: return "NONE_LITERAL";
        case AST_UNDEFINED_LITERAL: return "UNDEFINED_LITERAL";
        case AST_ARRAY_LITERAL: return "ARRAY_LITERAL";
        case AST_STRING_LITERAL: return "STRING_LITERAL";
        case AST_STRUCT_LITERAL: return "STRUCT_LITERAL";
        case AST_TYPELESS_STRUCT_LITERAL: return "TYPELESS_STRUCT_LITERAL";
        case AST_STRUCT_LITERAL_ENTRY_LIST: return "STRUCT_LITERAL_ENTRY_LIST";
        case AST_STRUCT_LITERAL_ENTRY: return "STRUCT_LITERAL_ENTRY";
        case AST_MAP_LITERAL: return "MAP_LITERAL";
        case AST_MAP_LITERAL_ENTRY: return "MAP_LITERAL_ENTRY";
        case AST_CLOSURE_LITERAL: return "CLOSURE_LITERAL";
        case AST_ADD_EXPR: return "ADD_EXPR";
        case AST_SUB_EXPR: return "SUB_EXPR";
        case AST_MUL_EXPR: return "MUL_EXPR";
        case AST_DIV_EXPR: return "DIV_EXPR";
        case AST_MOD_EXPR: return "MOD_EXPR";
        case AST_BIT_AND_EXPR: return "BIT_AND_EXPR";
        case AST_BIT_OR_EXPR: return "BIT_OR_EXPR";
        case AST_BIT_XOR_EXPR: return "BIT_XOR_EXPR";
        case AST_SHL_EXPR: return "SHL_EXPR";
        case AST_SHR_EXPR: return "SHR_EXPR";
        case AST_EQ_EXPR: return "EQ_EXPR";
        case AST_NE_EXPR: return "NE_EXPR";
        case AST_GT_EXPR: return "GT_EXPR";
        case AST_LT_EXPR: return "LT_EXPR";
        case AST_GE_EXPR: return "GE_EXPR";
        case AST_LE_EXPR: return "LE_EXPR";
        case AST_AND_EXPR: return "AND_EXPR";
        case AST_OR_EXPR: return "OR_EXPR";
        case AST_NEG_EXPR: return "NEG_EXPR";
        case AST_BIT_NOT_EXPR: return "BIT_NOT_EXPR";
        case AST_ADDR_EXPR: return "REF_EXPR";
        case AST_STAR_EXPR: return "STAR_EXPR";
        case AST_NOT_EXPR: return "NOT_EXPR";
        case AST_CAST_EXPR: return "CAST_EXPR";
        case AST_CALL_EXPR: return "CALL_EXPR";
        case AST_ARG_LIST: return "ARG_LIST";
        case AST_DOT_EXPR: return "DOT_EXPR";
        case AST_IMPLICIT_DOT_EXPR: return "IMPLICIT_DOT_EXPR";
        case AST_RANGE_EXPR: return "RANGE_EXPR";
        case AST_REF_EXPR: return "REF_EXPR";
        case AST_REF_MUT_EXPR: return "REF_MUT_EXPR";
        case AST_SHARE_EXPR: return "SHARE_EXPR";
        case AST_TRY_EXPR: return "TRY_EXPR";
        case AST_TUPLE_EXPR: return "TUPLE_EXPR";
        case AST_BRACKET_EXPR: return "BRACKET_EXPR";
        case AST_PAREN_EXPR: return "PAREN_EXPR";
        case AST_I8: return "I8";
        case AST_I16: return "I16";
        case AST_I32: return "I32";
        case AST_I64: return "I64";
        case AST_U8: return "U8";
        case AST_U16: return "U16";
        case AST_U32: return "U32";
        case AST_U64: return "U64";
        case AST_F32: return "F32";
        case AST_F64: return "F64";
        case AST_USIZE: return "USIZE";
        case AST_BOOL: return "BOOL";
        case AST_ADDR: return "ADDR";
        case AST_VOID: return "VOID";
        case AST_STATIC_ARRAY_TYPE: return "STATIC_ARRAY_TYPE";
        case AST_FUNC_TYPE: return "FUNC_TYPE";
        case AST_OPTIONAL_TYPE: return "OPTIONAL_TYPE";
        case AST_RESULT_TYPE: return "RESULT_TYPE";
        case AST_CLOSURE_TYPE: return "CLOSURE_TYPE";
        case AST_PARAM_SEQUENCE_TYPE: return "PARAM_SEQUENCE_TYPE";
        case AST_META_ACCESS: return "META_ACCESS";
        case AST_IDENTIFIER: return "IDENTIFIER";
        case AST_BLOCK: return "BLOCK";
        case AST_PARAM_LIST: return "PARAM_LIST";
        case AST_PARAM: return "PARAM";
        case AST_REF_PARAM: return "REF_PARAM";
        case AST_REF_MUT_PARAM: return "REF_MUT_PARAM";
        case AST_REF_RETURN: return "REF_RETURN";
        case AST_REF_MUT_RETURN: return "REF_MUT_RETURN";
        case AST_GENERIC_PARAM_LIST: return "GENERIC_PARAM_LIST";
        case AST_GENERIC_PARAMETER: return "GENERIC_PARAMETER";
        case AST_SELF: return "SELF";
        case AST_QUALIFIER_LIST: return "QUALIFIER_LIST";
        case AST_QUALIFIER: return "QUALIFIER";
        case AST_ATTRIBUTE_WRAPPER: return "ATTRIBUTE_WRAPPER";
        case AST_ATTRIBUTE_LIST: return "ATTRIBUTE_LIST";
        case AST_ATTRIBUTE_TAG: return "ATTRIBUTE_TAG";
        case AST_ATTRIBUTE_VALUE: return "ATTRIBUTE_VALUE";
        case AST_EMPTY: return "EMPTY";
        case AST_ERROR: return "ERROR";
        case AST_COMPLETION_TOKEN: return "COMPLETION_TOKEN";
        case AST_INVALID: return "INVALID";
    }
}

} // namespace lang

} // namespace banjo
