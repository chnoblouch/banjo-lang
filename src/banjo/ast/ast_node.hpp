#ifndef BANJO_AST_NODE_H
#define BANJO_AST_NODE_H

#include "banjo/lexer/token.hpp"
#include "banjo/source/text_range.hpp"

#include <cassert>
#include <cstdint>
#include <span>
#include <string_view>

namespace banjo {

namespace lang {

enum ASTNodeType : std::uint8_t {
    AST_MODULE,
    AST_FUNC_DEF,
    AST_GENERIC_FUNC_DEF,
    AST_FUNC_DECL,
    AST_NATIVE_FUNC_DECL,
    AST_CONST_DEF,
    AST_STRUCT_DEF,
    AST_GENERIC_STRUCT_DEF,
    AST_VAR_DEF,
    AST_TYPELESS_VAR_DEF,
    AST_REF_VAR_DEF,
    AST_TYPELESS_REF_VAR_DEF,
    AST_REF_MUT_VAR_DEF,
    AST_TYPELESS_REF_MUT_VAR_DEF,
    AST_NATIVE_VAR_DECL,
    AST_ENUM_DEF,
    AST_ENUM_VARIANT_LIST,
    AST_ENUM_VARIANT,
    AST_UNION_DEF,
    AST_UNION_CASE,
    AST_PROTO_DEF,
    AST_IMPL_LIST,
    AST_TYPE_ALIAS_DEF,
    AST_USE_DECL,
    AST_USE_LIST,
    AST_USE_REBIND,
    AST_EXPR_STMT,
    AST_ASSIGN_STMT,
    AST_ADD_ASSIGN_STMT,
    AST_SUB_ASSIGN_STMT,
    AST_MUL_ASSIGN_STMT,
    AST_DIV_ASSIGN_STMT,
    AST_MOD_ASSIGN_STMT,
    AST_BIT_AND_ASSIGN_STMT,
    AST_BIT_OR_ASSIGN_STMT,
    AST_BIT_XOR_ASSIGN_STMT,
    AST_SHL_ASSIGN_STMT,
    AST_SHR_ASSIGN_STMT,
    AST_RETURN_STMT,
    AST_IF_STMT,
    AST_IF_BRANCH,
    AST_ELSE_IF_BRANCH,
    AST_ELSE_BRANCH,
    AST_SWITCH_STMT,
    AST_SWITCH_CASE_LIST,
    AST_SWITCH_CASE_BRANCH,
    AST_SWITCH_DEFAULT_BRANCH,
    AST_TRY_STMT,
    AST_TRY_SUCCESS_BRANCH,
    AST_TRY_EXCEPT_BRANCH,
    AST_TRY_ELSE_BRANCH,
    AST_WHILE_STMT,
    AST_FOR_STMT,
    AST_FOR_REF_STMT,
    AST_FOR_REF_MUT_STMT,
    AST_CONTINUE_STMT,
    AST_BREAK_STMT,
    AST_META_IF_STMT,
    AST_META_IF_BRANCH,
    AST_META_ELSE_IF_BRANCH,
    AST_META_ELSE_BRANCH,
    AST_META_FOR_STMT,
    AST_INT_LITERAL,
    AST_FP_LITERAL,
    AST_FALSE_LITERAL,
    AST_TRUE_LITERAL,
    AST_CHAR_LITERAL,
    AST_NULL_LITERAL,
    AST_NONE_LITERAL,
    AST_UNDEFINED_LITERAL,
    AST_ARRAY_LITERAL,
    AST_STRING_LITERAL,
    AST_STRUCT_LITERAL,
    AST_TYPELESS_STRUCT_LITERAL,
    AST_STRUCT_LITERAL_ENTRY_LIST,
    AST_STRUCT_LITERAL_ENTRY,
    AST_MAP_LITERAL,
    AST_MAP_LITERAL_ENTRY,
    AST_CLOSURE_LITERAL,
    AST_ADD_EXPR,
    AST_SUB_EXPR,
    AST_MUL_EXPR,
    AST_DIV_EXPR,
    AST_MOD_EXPR,
    AST_BIT_AND_EXPR,
    AST_BIT_OR_EXPR,
    AST_BIT_XOR_EXPR,
    AST_SHL_EXPR,
    AST_SHR_EXPR,
    AST_EQ_EXPR,
    AST_NE_EXPR,
    AST_GT_EXPR,
    AST_LT_EXPR,
    AST_GE_EXPR,
    AST_LE_EXPR,
    AST_AND_EXPR,
    AST_OR_EXPR,
    AST_NEG_EXPR,
    AST_BIT_NOT_EXPR,
    AST_REF_EXPR,
    AST_STAR_EXPR,
    AST_NOT_EXPR,
    AST_CAST_EXPR,
    AST_CALL_EXPR,
    AST_ARG_LIST,
    AST_DOT_EXPR,
    AST_IMPLICIT_DOT_EXPR,
    AST_RANGE_EXPR,
    AST_TRY_EXPR,
    AST_TUPLE_EXPR,
    AST_BRACKET_EXPR,
    AST_PAREN_EXPR,
    AST_I8,
    AST_I16,
    AST_I32,
    AST_I64,
    AST_U8,
    AST_U16,
    AST_U32,
    AST_U64,
    AST_F32,
    AST_F64,
    AST_USIZE,
    AST_BOOL,
    AST_ADDR,
    AST_VOID,
    AST_STATIC_ARRAY_TYPE,
    AST_FUNC_TYPE,
    AST_OPTIONAL_TYPE,
    AST_RESULT_TYPE,
    AST_CLOSURE_TYPE,
    AST_PARAM_SEQUENCE_TYPE,
    AST_META_ACCESS,
    AST_IDENTIFIER,
    AST_BLOCK,
    AST_PARAM_LIST,
    AST_PARAM,
    AST_REF_PARAM,
    AST_REF_MUT_PARAM,
    AST_REF_RETURN,
    AST_REF_MUT_RETURN,
    AST_GENERIC_PARAM_LIST,
    AST_GENERIC_PARAMETER,
    AST_SELF,
    AST_QUALIFIER_LIST,
    AST_QUALIFIER,
    AST_ATTRIBUTE_WRAPPER,
    AST_ATTRIBUTE_LIST,
    AST_ATTRIBUTE_TAG,
    AST_ATTRIBUTE_VALUE,
    AST_EMPTY,
    AST_ERROR,
    AST_COMPLETION_TOKEN,
    AST_INVALID,
};

struct ASTNode {
    ASTNodeType type;
    std::string_view value;
    TextRange range;
    std::span<unsigned> tokens;

    ASTNode *first_child = nullptr;
    ASTNode *last_child = nullptr;
    ASTNode *next_sibling = nullptr;
    ASTNode *parent = nullptr;

    ASTNode();
    ASTNode(ASTNodeType type);
    ASTNode(ASTNodeType type, std::string_view value, TextRange range = {0, 0});
    ASTNode(ASTNodeType type, TextRange range);
    ASTNode(ASTNodeType type, Token *token);

    bool has_children() { return first_child; }
    unsigned num_children();
    void append_child(ASTNode *child);
    void set_range_from_children();
};

} // namespace lang

} // namespace banjo

#endif
