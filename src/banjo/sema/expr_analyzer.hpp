#ifndef BANJO_SEMA_EXPR_ANALYZER_H
#define BANJO_SEMA_EXPR_ANALYZER_H

#include "banjo/sema/semantic_analyzer.hpp"
#include "banjo/sir/sir.hpp"

namespace banjo {

namespace lang {

namespace sema {

class ExprAnalyzer {

public:
    static constexpr unsigned DONT_EVAL_META_EXPRS = 0x00000001;
    static constexpr unsigned ANALYZE_SYMBOL_INTERFACES = 0x00000002;

private:
    SemanticAnalyzer &analyzer;
    unsigned flags;

public:
    ExprAnalyzer(SemanticAnalyzer &analyzer, unsigned flags = 0x00000000);
    Result analyze_value(sir::Expr &expr);
    Result analyze_value(sir::Expr &expr, sir::Expr expected_type);
    Result analyze_value_uncoerced(sir::Expr &expr);
    Result analyze_type(sir::Expr &expr);

    Result analyze(sir::Expr &expr);
    Result analyze(sir::Expr &expr, sir::Expr expected_type);
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
    Result analyze_func_type(sir::FuncType &func_type);
    Result analyze_optional_type(sir::OptionalType &optional_type, sir::Expr &out_expr);
    Result analyze_result_type(sir::ResultType &result_type, sir::Expr &out_expr);
    Result analyze_array_type(sir::ArrayType &array_type, sir::Expr &out_expr);
    Result analyze_map_type(sir::MapType &map_type, sir::Expr &out_expr);
    Result analyze_closure_type(sir::ClosureType &closure_type);
    Result analyze_reference_type(sir::ReferenceType &reference_type);
    Result analyze_ident_expr(sir::IdentExpr &ident_expr, sir::Expr &out_expr);
    Result analyze_star_expr(sir::StarExpr &star_expr, sir::Expr &out_expr);
    Result analyze_bracket_expr(sir::BracketExpr &bracket_expr, sir::Expr &out_expr);
    Result analyze_dot_expr(sir::DotExpr &dot_expr, sir::Expr &out_expr);
    Result analyze_meta_access(sir::MetaAccess &meta_access);
    Result analyze_meta_field_expr(sir::MetaFieldExpr &meta_field_expr);
    Result analyze_meta_call_expr(sir::MetaCallExpr &meta_call_expr);

    Result analyze_completion_token();

    Result analyze_dot_expr_rhs(sir::DotExpr &dot_expr, sir::Expr &out_expr);
    Result analyze_index_expr(sir::BracketExpr &bracket_expr, sir::Expr base_type, sir::Expr &out_expr);
    Result analyze_operator_overload_call(sir::Symbol symbol, std::span<sir::Expr> args, sir::Expr &inout_expr);
    Result finalize_call_expr_args(sir::CallExpr &call_expr, sir::FuncType &func_type, sir::FuncDef *func_def);
    Result specialize(sir::FuncDef &func_def, std::span<sir::Expr> generic_args, sir::Expr &inout_expr);
    Result specialize(sir::StructDef &struct_def, std::span<sir::Expr> generic_args, sir::Expr &inout_expr);

    void create_method_call(sir::CallExpr &call_expr, sir::Expr lhs, const sir::Ident &rhs, sir::Symbol method);
    sir::Expr create_isize_cast(sir::Expr value);
    bool can_be_coerced(sir::Expr value);
};

} // namespace sema

} // namespace lang

} // namespace banjo

#endif
