#ifndef EXPR_FINALIZER_H
#define EXPR_FINALIZER_H

#include "banjo/sema2/semantic_analyzer.hpp"
#include "banjo/sir/sir.hpp"

namespace banjo {

namespace lang {

namespace sema {

class ExprFinalizer {

private:
    SemanticAnalyzer &analyzer;

public:
    ExprFinalizer(SemanticAnalyzer &analyzer);

    Result finalize_type_by_coercion(sir::Expr &expr, sir::Expr expected_type);
    Result finalize_type(sir::Expr &expr);

    Result coerce_to_union(sir::Expr &inout_expr, sir::Expr union_type);
    Result coerce_to_std_optional(sir::Expr &inout_expr, sir::Specialization<sir::StructDef> &specialization);
    Result coerce_to_std_result(sir::Expr &inout_expr, sir::Specialization<sir::StructDef> &specialization);

    Result finalize_coercion(sir::IntLiteral &int_literal, sir::Expr type);
    Result finalize_coercion(sir::FPLiteral &fp_literal, sir::Expr type);
    Result finalize_coercion(sir::NullLiteral &null_literal, sir::Expr type);
    Result finalize_coercion(sir::ArrayLiteral &array_literal, sir::Expr type, sir::Expr &out_expr);
    Result finalize_coercion(sir::StringLiteral &string_literal, sir::Expr type, sir::Expr &out_expr);
    Result finalize_coercion(sir::StructLiteral &struct_literal, sir::Expr type);
    Result finalize_coercion(sir::TupleExpr &tuple_literal, sir::Expr type);
    Result finalize_coercion(sir::MapLiteral &map_literal, sir::Expr type, sir::Expr &out_expr);
    Result finalize_coercion(sir::UnaryExpr &unary_expr, sir::Expr type);

    Result finalize_default(sir::IntLiteral &int_literal);
    Result finalize_default(sir::FPLiteral &fp_literal);
    Result finalize_default(sir::NullLiteral &null_literal);
    Result finalize_default(sir::NoneLiteral &none_literal);
    Result finalize_default(sir::UndefinedLiteral &undefined_literal);
    Result finalize_default(sir::ArrayLiteral &array_literal, sir::Expr &out_expr);
    Result finalize_default(sir::StringLiteral &string_literal, sir::Expr &out_expr);
    Result finalize_default(sir::StructLiteral &struct_literal);
    Result finalize_default(sir::TupleExpr &tuple_literal);
    Result finalize_default(sir::MapLiteral &map_literal, sir::Expr &out_expr);
    Result finalize_default(sir::UnaryExpr &unary_expr);

    void create_std_string(sir::StringLiteral &string_literal, sir::Expr &out_expr);
    void create_std_array(sir::ArrayLiteral &array_literal, const sir::Expr &element_type, sir::Expr &out_expr);
    void create_std_optional_some(sir::Specialization<sir::StructDef> &specialization, sir::Expr &inout_expr);
    void create_std_optional_none(sir::Specialization<sir::StructDef> &specialization, sir::Expr &out_expr);
    void create_std_result_success(sir::Specialization<sir::StructDef> &specialization, sir::Expr &inout_expr);
    void create_std_result_failure(sir::Specialization<sir::StructDef> &specialization, sir::Expr &inout_expr);
    void create_std_map(sir::MapLiteral &map_literal, sir::Expr &out_expr);
    Result finalize_array_literal_elements(sir::ArrayLiteral &array_literal, sir::Expr element_type);
    Result finalize_struct_literal_fields(sir::StructLiteral &struct_literal);
    Result finalize_map_literal_elements(sir::MapLiteral &map_literal, sir::Expr key_type, sir::Expr value_type);
};

} // namespace sema

} // namespace lang

} // namespace banjo

#endif
