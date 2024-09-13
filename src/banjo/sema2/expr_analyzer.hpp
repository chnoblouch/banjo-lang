#ifndef EXPR_ANALYZER_H
#define EXPR_ANALYZER_H

#include "banjo/sema2/semantic_analyzer.hpp"
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
    ExprConstraints constraints;

public:
    ExprAnalyzer(SemanticAnalyzer &analyzer);
    ExprAnalyzer(SemanticAnalyzer &analyzer, ExprConstraints constraints);
    Result analyze(sir::Expr &expr);

private:
    Result analyze_uncoerced(sir::Expr &expr);
    Result finalize_type_by_coercion(sir::Expr &expr, sir::Expr expected_type);
    Result finalize_type(sir::Expr &expr);

    Result analyze_int_literal(sir::IntLiteral &int_literal);
    Result analyze_fp_literal(sir::FPLiteral &fp_literal);
    Result analyze_bool_literal(sir::BoolLiteral &bool_literal);
    Result analyze_char_literal(sir::CharLiteral &char_literal);
    Result analyze_array_literal(sir::ArrayLiteral &array_literal, sir::Expr &out_expr);
    Result analyze_string_literal(sir::StringLiteral &string_literal);
    Result analyze_struct_literal(sir::StructLiteral &struct_literal);
    Result analyze_closure_literal(sir::ClosureLiteral &closure_literal, sir::Expr &out_expr);
    Result analyze_binary_expr(sir::BinaryExpr &binary_expr, sir::Expr &out_expr);
    Result analyze_unary_expr(sir::UnaryExpr &unary_expr, sir::Expr &out_expr);
    void analyze_cast_expr(sir::CastExpr &cast_expr);
    Result analyze_call_expr(sir::CallExpr &call_expr, sir::Expr &out_expr);
    Result analyze_dot_expr_callee(sir::DotExpr &dot_expr, sir::CallExpr &out_call_expr, bool &is_method);
    Result analyze_union_case_literal(sir::CallExpr &call_expr, sir::Expr &out_expr);
    void analyze_range_expr(sir::RangeExpr &range_expr);
    void analyze_tuple_expr(sir::TupleExpr &tuple_expr);
    Result analyze_static_array_type(sir::StaticArrayType &static_array_type);
    void analyze_func_type(sir::FuncType &func_type);
    Result analyze_optional_type(sir::OptionalType &optional_type, sir::Expr &out_expr);
    Result analyze_result_type(sir::ResultType &result_type, sir::Expr &out_expr);
    Result analyze_array_type(sir::ArrayType &array_type, sir::Expr &out_expr);
    Result analyze_closure_type(sir::ClosureType &closure_type);
    Result analyze_dot_expr(sir::DotExpr &dot_expr, sir::Expr &out_expr);
    Result analyze_ident_expr(sir::IdentExpr &ident_expr, sir::Expr &out_expr);
    Result analyze_star_expr(sir::StarExpr &star_expr, sir::Expr &out_expr);
    Result analyze_bracket_expr(sir::BracketExpr &bracket_expr, sir::Expr &out_expr);

    void create_std_string(sir::StringLiteral &string_literal, sir::Expr &out_expr);
    void create_std_array(sir::ArrayLiteral &array_literal, const sir::Expr &element_type, sir::Expr &out_expr);
    void create_std_optional_some(sir::Specialization<sir::StructDef> &specialization, sir::Expr &inout_expr);
    void create_std_optional_none(sir::Specialization<sir::StructDef> &specialization, sir::Expr &out_expr);
    void create_std_result_success(sir::Specialization<sir::StructDef> &specialization, sir::Expr &inout_expr);
    void create_std_result_failure(sir::Specialization<sir::StructDef> &specialization, sir::Expr &inout_expr);
    Result finalize_array_literal_elements(sir::ArrayLiteral &array_literal, sir::Expr element_type);
    Result finalize_struct_literal_fields(sir::StructLiteral &struct_literal);
    Result analyze_dot_expr_rhs(sir::DotExpr &dot_expr, sir::Expr &out_expr);
    sir::FuncDef *resolve_overload(sir::OverloadSet &overload_set, const std::vector<sir::Expr> &args);
    bool is_matching_overload(sir::FuncDef &func_def, const std::vector<sir::Expr> &args);
    Result analyze_operator_overload_call(sir::Symbol symbol, sir::Expr self, sir::Expr arg, sir::Expr &out_expr);
    Result specialize(sir::FuncDef &func_def, const std::vector<sir::Expr> &generic_args, sir::Expr &inout_expr);
    Result specialize(sir::StructDef &struct_def, const std::vector<sir::Expr> &generic_args, sir::Expr &inout_expr);
};

} // namespace sema

} // namespace lang

} // namespace banjo

#endif
