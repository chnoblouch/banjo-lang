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
        case AST_BLOCK: return "BLOCK";
        case AST_IDENTIFIER: return "IDENTIFIER";
        case AST_VAR: return "VAR";
        case AST_REF_VAR: return "REF_VAR";
        case AST_IMPLICIT_TYPE_REF_VAR: return "IMPLICIT_TYPE_REF_VAR";
        case AST_REF_MUT_VAR: return "REF_MUT_VAR";
        case AST_IMPLICIT_TYPE_REF_MUT_VAR: return "IMPLICIT_TYPE_REF_MUT_VAR";
        case AST_IMPLICIT_TYPE_VAR: return "IMPLICIT_TYPE_VAR";
        case AST_CONSTANT: return "CONSTANT";
        case AST_ASSIGNMENT: return "ASSIGNMENT";
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
        case AST_OPTIONAL_DATA_TYPE: return "OPTIONAL_DATA_TYPE";
        case AST_RESULT_TYPE: return "RESULT_TYPE";
        case AST_FUNCTION_DATA_TYPE: return "FUNCTION_DATA_TYPE";
        case AST_STATIC_ARRAY_TYPE: return "STATIC_ARRAY_TYPE";
        case AST_CLOSURE_TYPE: return "CLOSURE_TYPE";
        case AST_PARAM_SEQUENCE_TYPE: return "PARAM_SEQUENCE_TYPE";
        case AST_INT_LITERAL: return "INT_LITERAL";
        case AST_FLOAT_LITERAL: return "FLOAT_LITERAL";
        case AST_CHAR_LITERAL: return "CHAR_LITERAL";
        case AST_STRING_LITERAL: return "STRING_LITERAL";
        case AST_ARRAY_EXPR: return "ARRAY_EXPR";
        case AST_MAP_EXPR: return "MAP_EXPR";
        case AST_MAP_EXPR_PAIR: return "MAP_EXPR_PAIR";
        case AST_CLOSURE: return "CLOSURE";
        case AST_FALSE: return "FALSE";
        case AST_TRUE: return "TRUE";
        case AST_NULL: return "NULL";
        case AST_NONE: return "NONE";
        case AST_UNDEFINED: return "UNDEFINED";
        case AST_OPERATOR_ADD: return "OPERATOR_ADD";
        case AST_OPERATOR_SUB: return "OPERATOR_SUB";
        case AST_OPERATOR_MUL: return "OPERATOR_MUL";
        case AST_OPERATOR_DIV: return "OPERATOR_DIV";
        case AST_OPERATOR_MOD: return "OPERATOR_MOD";
        case AST_OPERATOR_BIT_AND: return "OPERATOR_BIT_AND";
        case AST_OPERATOR_BIT_OR: return "OPERATOR_BIT_OR";
        case AST_OPERATOR_BIT_XOR: return "OPERATOR_BIT_XOR";
        case AST_OPERATOR_SHL: return "OPERATOR_SHL";
        case AST_OPERATOR_SHR: return "OPERATOR_SHR";
        case AST_OPERATOR_EQ: return "OPERATOR_EQ";
        case AST_OPERATOR_NE: return "OPERATOR_NE";
        case AST_OPERATOR_GT: return "OPERATOR_GT";
        case AST_OPERATOR_LT: return "OPERATOR_LT";
        case AST_OPERATOR_GE: return "OPERATOR_GE";
        case AST_OPERATOR_LE: return "OPERATOR_LE";
        case AST_OPERATOR_AND: return "OPERATOR_AND";
        case AST_OPERATOR_OR: return "OPERATOR_OR";
        case AST_DOT_OPERATOR: return "DOT_OPERATOR";
        case AST_IMPLICIT_DOT_OPERATOR: return "IMPLICIT_DOT_OPERATOR";
        case AST_OPERATOR_NEG: return "OPERATOR_NEG";
        case AST_OPERATOR_REF: return "OPERATOR_REF";
        case AST_STAR_EXPR: return "STAR_EXPR";
        case AST_OPERATOR_NOT: return "OPERATOR_NOT";
        case AST_OPERATOR_BIT_NOT: return "OPERATOR_BIT_NOT";
        case AST_CAST: return "CAST";
        case AST_RANGE: return "RANGE";
        case AST_ADD_ASSIGN: return "ADD_ASSIGN";
        case AST_SUB_ASSIGN: return "SUB_ASSIGN";
        case AST_MUL_ASSIGN: return "MUL_ASSIGN";
        case AST_DIV_ASSIGN: return "DIV_ASSIGN";
        case AST_MOD_ASSIGN: return "MOD_ASSIGN";
        case AST_BIT_AND_ASSIGN: return "BIT_AND_ASSIGN";
        case AST_BIT_OR_ASSIGN: return "BIT_OR_ASSIGN";
        case AST_BIT_XOR_ASSIGN: return "BIT_XOR_ASSIGN";
        case AST_SHL_ASSIGN: return "SHL_ASSIGN";
        case AST_SHR_ASSIGN: return "SHR_ASSIGN";
        case AST_ARRAY_ACCESS: return "ARRAY_ACCESS";
        case AST_TUPLE_EXPR: return "TUPLE_EXPR";
        case AST_IF_CHAIN: return "IF_CHAIN";
        case AST_IF: return "IF";
        case AST_ELSE_IF: return "ELSE_IF";
        case AST_ELSE: return "ELSE";
        case AST_SWITCH: return "SWITCH";
        case AST_SWITCH_CASE_LIST: return "SWITCH_CASE_LIST";
        case AST_SWITCH_CASE: return "SWITCH_CASE";
        case AST_SWITCH_DEFAULT_CASE: return "SWITCH_DEFAULT_CASE";
        case AST_TRY: return "TRY";
        case AST_TRY_SUCCESS_CASE: return "TRY_SUCCESS_CASE";
        case AST_TRY_ERROR_CASE: return "TRY_ERROR_CASE";
        case AST_TRY_ELSE_CASE: return "TRY_ELSE_CASE";
        case AST_WHILE: return "WHILE";
        case AST_FOR: return "FOR";
        case AST_FOR_REF: return "FOR_REF";
        case AST_FOR_REF_MUT: return "FOR_REF_MUT";
        case AST_BREAK: return "BREAK";
        case AST_CONTINUE: return "CONTINUE";
        case AST_FUNCTION_DEFINITION: return "FUNCTION_DEFINITION";
        case AST_FUNC_DECL: return "FUNC_DECL";
        case AST_PARAM_LIST: return "PARAM_LIST";
        case AST_PARAM: return "PARAM";
        case AST_REF_PARAM: return "REF_PARAM";
        case AST_REF_MUT_PARAM: return "REF_MUT_PARAM";
        case AST_REF_RETURN: return "REF_RETURN";
        case AST_REF_MUT_RETURN: return "REF_MUT_RETURN";
        case AST_GENERIC_FUNCTION_DEFINITION: return "GENERIC_FUNCTION_DEFINITION";
        case AST_GENERIC_STRUCT_DEFINITION: return "GENERIC_STRUCT_DEFINITION";
        case AST_GENERIC_PARAM_LIST: return "GENERIC_PARAM_LIST";
        case AST_GENERIC_PARAMETER: return "GENERIC_PARAMETER";
        case AST_FUNCTION_CALL: return "FUNCTION_CALL";
        case AST_FUNCTION_ARGUMENT_LIST: return "FUNCTION_ARGUMENT_LIST";
        case AST_FUNCTION_RETURN: return "FUNCTION_RETURN";
        case AST_STRUCT_DEFINITION: return "STRUCT_DEFINITION";
        case AST_SELF: return "SELF";
        case AST_STRUCT_INSTANTIATION: return "STRUCT_INSTANTIATION";
        case AST_ANON_STRUCT_LITERAL: return "ANON_STRUCT_LITERAL";
        case AST_STRUCT_FIELD_VALUE_LIST: return "STRUCT_FIELD_VALUE_LIST";
        case AST_STRUCT_FIELD_VALUE: return "STRUCT_FIELD_VALUE";
        case AST_ENUM_DEFINITION: return "ENUM_DEFINITION";
        case AST_ENUM_VARIANT_LIST: return "ENUM_VARIANT_LIST";
        case AST_ENUM_VARIANT: return "ENUM_VARIANT";
        case AST_UNION: return "UNION";
        case AST_UNION_CASE: return "UNION_CASE";
        case AST_PROTO: return "PROTO";
        case AST_IMPL_LIST: return "IMPL_LIST";
        case AST_TYPE_ALIAS: return "TYPE_ALIAS";
        case AST_NATIVE_FUNCTION_DECLARATION: return "NATIVE_FUNCTION_DECLARATION";
        case AST_NATIVE_VAR: return "NATIVE_VAR";
        case AST_QUALIFIER_LIST: return "QUALIFIER_LIST";
        case AST_QUALIFIER: return "QUALIFIER";
        case AST_ATTRIBUTE_WRAPPER: return "ATTRIBUTE_WRAPPER";
        case AST_ATTRIBUTE_LIST: return "ATTRIBUTE_LIST";
        case AST_ATTRIBUTE_TAG: return "ATTRIBUTE_TAG";
        case AST_ATTRIBUTE_VALUE: return "ATTRIBUTE_VALUE";
        case AST_USE: return "USE";
        case AST_USE_TREE_LIST: return "USE_TREE_LIST";
        case AST_USE_REBINDING: return "USE_REBINDING";
        case AST_META_EXPR: return "META_EXPR";
        case AST_META_IF: return "META_IF";
        case AST_META_IF_CONDITION: return "META_IF_CONDITION";
        case AST_META_ELSE: return "META_ELSE";
        case AST_META_FOR: return "META_FOR";
        case AST_PAREN_EXPR: return "PAREN_EXPR";
        case AST_EMPTY: return "EMPTY";
        case AST_ERROR: return "ERROR";
        case AST_COMPLETION_TOKEN: return "COMPLETION_TOKEN";
        case AST_INVALID: return "INVALID";
    }
}

} // namespace lang

} // namespace banjo
