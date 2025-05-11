#ifndef EXPR_ANALYZER_H
#define EXPR_ANALYZER_H

#include "banjo/sema/semantic_analyzer.hpp"
#include "banjo/sir/sir.hpp"

namespace banjo {

namespace lang {

namespace sema {

struct ExprConstraints {
    sir::Expr expected_type = nullptr;

    static ExprConstraints expect_type(sir::Expr type);
};

class ExprAnalyzer {

private:
    SemanticAnalyzer &analyzer;

public:
    ExprAnalyzer(SemanticAnalyzer &analyzer);
    Result analyze_value(sir::Expr &expr, ExprConstraints constraints = {});
    Result analyze_value_uncoerced(sir::Expr &expr);
    Result analyze_type(sir::Expr &expr);

    Result analyze(sir::Expr &expr, ExprConstraints constraints = {});
    Result analyze_uncoerced(sir::Expr &expr);

private:
    Result analyze_int_literal(sir::IntLiteral &int_literal);
    Result analyze_fp_literal(sir::FPLiteral &fp_literal);
    Result analyze_bool_literal(sir::BoolLiteral &bool_literal);
    Result analyze_char_literal(sir::CharLiteral &char_literal);
    Result analyze_null_literal(sir::NullLiteral &null_literal);
    Result analyze_none_literal(sir::NoneLiteral &none_literal);
    Result analyze_undefined_literal(sir::UndefinedLiteral &undefined_literal);
    Result analyze_array_literal(sir::ArrayLiteral &array_literal, sir::Expr &out_expr);
    Result analyze_string_literal(sir::StringLiteral &string_literal);
    Result analyze_struct_literal(sir::StructLiteral &struct_literal);
    Result analyze_map_literal(sir::MapLiteral &map_literal, sir::Expr &out_expr);
    Result analyze_closure_literal(sir::ClosureLiteral &closure_literal, sir::Expr &out_expr);
    Result analyze_binary_expr(sir::BinaryExpr &binary_expr, sir::Expr &out_expr);
    Result analyze_unary_expr(sir::UnaryExpr &unary_expr, sir::Expr &out_expr);
    Result analyze_cast_expr(sir::CastExpr &cast_expr);
    Result analyze_call_expr(sir::CallExpr &call_expr, sir::Expr &out_expr);
    Result analyze_dot_expr_callee(sir::DotExpr &dot_expr, sir::CallExpr &out_call_expr, bool &is_method);
    Result analyze_union_case_literal(sir::CallExpr &call_expr, sir::Expr &out_expr);
    Result analyze_range_expr(sir::RangeExpr &range_expr);
    void analyze_tuple_expr(sir::TupleExpr &tuple_expr);
    Result analyze_static_array_type(sir::StaticArrayType &static_array_type);
    void analyze_func_type(sir::FuncType &func_type);
    Result analyze_optional_type(sir::OptionalType &optional_type, sir::Expr &out_expr);
    Result analyze_result_type(sir::ResultType &result_type, sir::Expr &out_expr);
    Result analyze_array_type(sir::ArrayType &array_type, sir::Expr &out_expr);
    Result analyze_map_type(sir::MapType &map_type, sir::Expr &out_expr);
    Result analyze_closure_type(sir::ClosureType &closure_type);
    Result analyze_reference_type(sir::ReferenceType &reference_type);
    Result analyze_dot_expr(sir::DotExpr &dot_expr, sir::Expr &out_expr);
    Result analyze_ident_expr(sir::IdentExpr &ident_expr, sir::Expr &out_expr);
    Result analyze_star_expr(sir::StarExpr &star_expr, sir::Expr &out_expr);
    Result analyze_bracket_expr(sir::BracketExpr &bracket_expr, sir::Expr &out_expr);

    Result analyze_completion_token();

    void create_method_call(
        sir::CallExpr &call_expr,
        sir::Expr lhs,
        sir::Ident rhs,
        sir::Symbol method,
        bool lhs_is_already_pointer
    );

    Result analyze_dot_expr_rhs(sir::DotExpr &dot_expr, sir::Expr &out_expr);
    Result analyze_index_expr(sir::BracketExpr &bracket_expr, sir::Expr base_type, sir::Expr &out_expr);
    Result analyze_operator_overload_call(sir::Symbol symbol, sir::Expr self, sir::Expr arg, sir::Expr &inout_expr);
    Result specialize(sir::FuncDef &func_def, const std::vector<sir::Expr> &generic_args, sir::Expr &inout_expr);
    Result specialize(sir::StructDef &struct_def, const std::vector<sir::Expr> &generic_args, sir::Expr &inout_expr);

    sir::Expr create_isize_cast(sir::Expr value);
    bool can_be_coerced(sir::Expr value);
};

} // namespace sema

} // namespace lang

} // namespace banjo

#endif
