#ifndef AST_CHILD_INDICES_H
#define AST_CHILD_INDICES_H

namespace lang {

constexpr unsigned VAR_QUALIFIERS = 0;
constexpr unsigned VAR_NAME = 0;
constexpr unsigned VAR_TYPE = 1;
constexpr unsigned VAR_VALUE = 2;

constexpr unsigned TYPE_INFERRED_VAR_QUALIFIERS = 0;
constexpr unsigned TYPE_INFERRED_VAR_NAME = 0;
constexpr unsigned TYPE_INFERRED_VAR_VALUE = 1;

constexpr unsigned CONST_QUALIFIERS = 0;
constexpr unsigned CONST_NAME = 0;
constexpr unsigned CONST_TYPE = 1;
constexpr unsigned CONST_VALUE = 2;

constexpr unsigned FUNC_QUALIFIERS = 0;
constexpr unsigned FUNC_NAME = 1;
constexpr unsigned FUNC_PARAMS = 2;
constexpr unsigned FUNC_TYPE = 3;
constexpr unsigned FUNC_BLOCK = 4;

constexpr unsigned PARAM_NAME = 0;
constexpr unsigned PARAM_TYPE = 1;

constexpr unsigned GENERIC_FUNC_QUALIFIERS = 0;
constexpr unsigned GENERIC_FUNC_NAME = 1;
constexpr unsigned GENERIC_FUNC_GENERIC_PARAMS = 2;
constexpr unsigned GENERIC_FUNC_PARAMS = 3;
constexpr unsigned GENERIC_FUNC_TYPE = 4;
constexpr unsigned GENERIC_FUNC_BLOCK = 5;

constexpr unsigned GENERIC_PARAM_NAME = 0;
constexpr unsigned GENERIC_PARAM_TYPE = 1;

constexpr unsigned GENERIC_INSTANTIATION_TEMPLATE = 0;
constexpr unsigned GENERIC_INSTANTIATION_ARGS = 1;

constexpr unsigned STRUCT_NAME = 0;
constexpr unsigned STRUCT_BLOCK = 1;

constexpr unsigned GENERIC_STRUCT_NAME = 0;
constexpr unsigned GENERIC_STRUCT_GENERIC_PARAMS = 1;
constexpr unsigned GENERIC_STRUCT_BLOCK = 2;

constexpr unsigned ENUM_NAME = 0;
constexpr unsigned ENUM_VARIANTS = 1;

constexpr unsigned ENUM_VARIANT_NAME = 0;
constexpr unsigned ENUM_VARIANT_VALUE = 1;

constexpr unsigned UNION_NAME = 0;
constexpr unsigned UNION_BLOCK = 1;

constexpr unsigned UNION_CASE_NAME = 0;
constexpr unsigned UNION_CASE_FIELDS = 1;

constexpr unsigned UNION_CASE_FIELD_NAME = 0;
constexpr unsigned UNION_CASE_FIELD_TYPE = 1;

constexpr unsigned TYPE_ALIAS_NAME = 0;
constexpr unsigned TYPE_ALIAS_UNDERLYING_TYPE = 1;

constexpr unsigned ASSIGN_LOCATION = 0;
constexpr unsigned ASSIGN_VALUE = 1;

constexpr unsigned IF_CONDITION = 0;
constexpr unsigned IF_BLOCK = 1;

constexpr unsigned SWITCH_VALUE = 0;
constexpr unsigned SWITCH_CASES = 1;

constexpr unsigned WHILE_CONDITION = 0;
constexpr unsigned WHILE_BLOCK = 1;

constexpr unsigned FOR_ITER_TYPE = 0;
constexpr unsigned FOR_VAR = 1;
constexpr unsigned FOR_EXPR = 2;
constexpr unsigned FOR_BLOCK = 3;

constexpr unsigned CALL_LOCATION = 0;
constexpr unsigned CALL_ARGS = 1;

constexpr unsigned USE_REBINDING_TARGET = 0;
constexpr unsigned USE_REBINDING_LOCAL = 1;

constexpr unsigned STRUCT_LITERAL_NAME = 0;
constexpr unsigned STRUCT_LITERAL_VALUES = 1;
constexpr unsigned STRUCT_FIELD_VALUE_NAME = 0;
constexpr unsigned STRUCT_FIELD_VALUE_VALUE = 1;

constexpr unsigned MAP_LITERAL_PAIR_KEY = 0;
constexpr unsigned MAP_LITERAL_PAIR_VALUE = 1;

constexpr unsigned CLOSURE_PARAMS = 0;
constexpr unsigned CLOSURE_RETURN_TYPE = 1;
constexpr unsigned CLOSURE_BLOCK = 2;

constexpr unsigned FUNC_TYPE_PARAMS = 0;
constexpr unsigned FUNC_TYPE_RETURN_TYPE = 1;

constexpr unsigned CLOSURE_TYPE_PARAMS = 0;
constexpr unsigned CLOSURE_TYPE_RETURN_TYPE = 1;

constexpr unsigned META_EXPR_KIND = 0;
constexpr unsigned META_EXPR_ARGS = 1;

} // namespace lang

#endif
