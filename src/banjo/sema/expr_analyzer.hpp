#ifndef EXPR_ANALYZER_H
#define EXPR_ANALYZER_H

#include "ast/ast_node.hpp"
#include "sema/semantic_analyzer_context.hpp"
#include "symbol/data_type.hpp"
#include "symbol/location.hpp"

namespace lang {

class ExprAnalyzer {

private:
    ASTNode *node;
    SemanticAnalyzerContext &context;
    DataType *expected_type = nullptr;

    DataType *type = nullptr;

public:
    ExprAnalyzer(ASTNode *node, SemanticAnalyzerContext &context);
    void set_expected_type(DataType *expected_type) { this->expected_type = expected_type; }
    bool check();
    DataType *get_type() { return type; }

private:
    bool check_int_literal();
    bool check_float_literal();
    bool check_char_literal();
    bool check_string_literal();
    bool check_array_literal();
    bool check_map_literal();
    bool check_struct_literal();
    bool check_anon_struct_literal();
    bool check_tuple_literal();
    bool check_closure();
    bool check_false();
    bool check_true();
    bool check_null();
    bool check_none();
    bool check_undefined();
    bool check_binary_operator(bool is_bool);
    bool check_operator_neg();
    bool check_operator_ref();
    bool check_operator_deref();
    bool check_operator_not();
    bool check_location();
    bool check_call();
    bool check_union_case_expr(const Location &location);
    bool check_function_call(const Location &location);
    bool check_array_access();
    bool check_cast();
    bool check_range();
    bool check_meta_expr();

    DataType *instantiate_std_generic_struct(const ModulePath &path, std::string name, GenericInstanceArgs args);
    bool check_any_struct_literal(Structure *struct_, ASTNode *values_node);
    Function *resolve_operator_overload(ASTNodeType op, Expr *lhs, Expr *rhs);

    bool is_implicit_cast();
    bool compare_against_expected_type();
    bool is_coercible_in_binary_operator(ASTNodeType type);

    bool is_std_string_expected();
    bool is_struct_expected();
};

} // namespace lang

#endif
