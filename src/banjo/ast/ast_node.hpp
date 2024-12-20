#ifndef AST_NODE_H
#define AST_NODE_H

#include "banjo/ast/attribute_list.hpp"
#include "banjo/lexer/token.hpp"
#include "banjo/source/text_range.hpp"

#include <cassert>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace banjo {

namespace lang {

class SymbolTable;
class Variable;
class NodeBuilder;

enum ASTNodeType {
    AST_MODULE,
    AST_BLOCK,
    AST_IDENTIFIER,
    AST_VAR,
    AST_IMPLICIT_TYPE_VAR,
    AST_CONSTANT,
    AST_ASSIGNMENT,
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
    AST_OPTIONAL_DATA_TYPE,
    AST_RESULT_TYPE,
    AST_FUNCTION_DATA_TYPE,
    AST_STATIC_ARRAY_TYPE,
    AST_CLOSURE_TYPE,
    AST_PARAM_SEQUENCE_TYPE,
    AST_INT_LITERAL,
    AST_FLOAT_LITERAL,
    AST_CHAR_LITERAL,
    AST_STRING_LITERAL,
    AST_ARRAY_EXPR,
    AST_MAP_EXPR,
    AST_MAP_EXPR_PAIR,
    AST_CLOSURE,
    AST_FALSE,
    AST_TRUE,
    AST_NULL,
    AST_NONE,
    AST_UNDEFINED,
    AST_OPERATOR_ADD,
    AST_OPERATOR_SUB,
    AST_OPERATOR_MUL,
    AST_OPERATOR_DIV,
    AST_OPERATOR_MOD,
    AST_OPERATOR_BIT_AND,
    AST_OPERATOR_BIT_OR,
    AST_OPERATOR_BIT_XOR,
    AST_OPERATOR_SHL,
    AST_OPERATOR_SHR,
    AST_OPERATOR_EQ,
    AST_OPERATOR_NE,
    AST_OPERATOR_GT,
    AST_OPERATOR_LT,
    AST_OPERATOR_GE,
    AST_OPERATOR_LE,
    AST_OPERATOR_AND,
    AST_OPERATOR_OR,
    AST_DOT_OPERATOR,
    AST_OPERATOR_NEG,
    AST_OPERATOR_REF,
    AST_STAR_EXPR,
    AST_OPERATOR_NOT,
    AST_CAST,
    AST_RANGE,
    AST_ADD_ASSIGN,
    AST_SUB_ASSIGN,
    AST_MUL_ASSIGN,
    AST_DIV_ASSIGN,
    AST_MOD_ASSIGN,
    AST_BIT_AND_ASSIGN,
    AST_BIT_OR_ASSIGN,
    AST_BIT_XOR_ASSIGN,
    AST_SHL_ASSIGN,
    AST_SHR_ASSIGN,
    AST_ARRAY_ACCESS,
    AST_TUPLE_EXPR,
    AST_IF_CHAIN,
    AST_IF,
    AST_ELSE_IF,
    AST_ELSE,
    AST_SWITCH,
    AST_SWITCH_CASE_LIST,
    AST_SWITCH_CASE,
    AST_SWITCH_DEFAULT_CASE,
    AST_TRY,
    AST_TRY_SUCCESS_CASE,
    AST_TRY_ERROR_CASE,
    AST_TRY_ELSE_CASE,
    AST_WHILE,
    AST_FOR,
    AST_FOR_ITER_TYPE,
    AST_BREAK,
    AST_CONTINUE,
    AST_FUNCTION_DEFINITION,
    AST_FUNC_DECL,
    AST_PARAM_LIST,
    AST_PARAM,
    AST_GENERIC_FUNCTION_DEFINITION,
    AST_GENERIC_STRUCT_DEFINITION,
    AST_GENERIC_PARAM_LIST,
    AST_GENERIC_PARAMETER,
    AST_FUNCTION_CALL,
    AST_FUNCTION_ARGUMENT_LIST,
    AST_FUNCTION_RETURN,
    AST_STRUCT_DEFINITION,
    AST_SELF,
    AST_STRUCT_INSTANTIATION,
    AST_ANON_STRUCT_LITERAL,
    AST_STRUCT_FIELD_VALUE_LIST,
    AST_STRUCT_FIELD_VALUE,
    AST_ENUM_DEFINITION,
    AST_ENUM_VARIANT_LIST,
    AST_ENUM_VARIANT,
    AST_UNION,
    AST_UNION_CASE,
    AST_PROTO,
    AST_IMPL_LIST,
    AST_TYPE_ALIAS,
    AST_NATIVE_FUNCTION_DECLARATION,
    AST_NATIVE_VAR,
    AST_QUALIFIER_LIST,
    AST_QUALIFIER,
    AST_USE,
    AST_USE_TREE_LIST,
    AST_USE_REBINDING,
    AST_META_EXPR,
    AST_META_IF,
    AST_META_IF_CONDITION,
    AST_META_ELSE,
    AST_META_FOR,
    AST_EXPLICIT_TYPE,
    AST_META_FIELD_ACCESS,
    AST_META_METHOD_CALL,
    AST_PAREN_EXPR,
    AST_EMPTY_LINE,
    AST_ERROR,
    AST_COMPLETION_TOKEN,
    AST_INVALID,
};

class ASTNode {

public:
    struct Flags {
        bool trailing_comma : 1 = false;
    };

protected:
    ASTNodeType type;
    std::string value;
    TextRange range;
    std::unique_ptr<AttributeList> attribute_list = nullptr;

    std::vector<ASTNode *> children;
    ASTNode *parent = nullptr;

public:
    Flags flags;

public:
    ASTNode();
    ASTNode(ASTNodeType type);
    ASTNode(ASTNodeType type, std::string value, TextRange range = {0, 0});
    ASTNode(ASTNodeType type, TextRange range);
    ASTNode(ASTNodeType type, Token *token);
    ~ASTNode();

    ASTNodeType get_type() { return type; }
    TextRange get_range() { return range; }
    AttributeList *get_attribute_list() { return attribute_list.get(); }

    const std::string &get_value() { return value; }
    void set_range(TextRange range) { this->range = range; }

    std::vector<ASTNode *> &get_children() { return children; }
    ASTNode *get_child(int index) { return children[index]; }
    ASTNode *get_child() { return children[0]; }
    bool has_child(unsigned index);
    ASTNode *get_parent() { return parent; }

    void set_type(ASTNodeType type) { this->type = type; }
    void set_value(std::string value) { this->value = std::move(value); }
    void set_parent(ASTNode *parent) { this->parent = parent; }

    void append_child(ASTNode *child);

    void set_range_from_children();

    template <typename T>
    T *as() {
        return static_cast<T *>(this);
    }

    friend class NodeBuilder;
};

} // namespace lang

} // namespace banjo

#endif
