#ifndef REPORT_GENERATOR_H
#define REPORT_GENERATOR_H

#include "banjo/reports/report.hpp"
#include "banjo/sir/sir.hpp"

#include <string_view>

namespace banjo {

namespace lang {

namespace sema {

class SemanticAnalyzer;

class ReportBuilder {

private:
    SemanticAnalyzer &analyzer;
    Report partial_report;

public:
    ReportBuilder(SemanticAnalyzer &analyzer, Report::Type type);

    template <typename... FormatArgs>
    ReportBuilder &set_message(std::string_view format_str, ASTNode *node, FormatArgs... format_args);

    template <typename... FormatArgs>
    ReportBuilder &add_note(std::string_view format_str, ASTNode *node, FormatArgs... format_args);

    SourceLocation find_location(ASTNode *node);

    void report();

private:
    const ModulePath &find_mod_path(ASTNode *node);
};

class ReportGenerator {

private:
    SemanticAnalyzer &analyzer;

public:
    ReportGenerator(SemanticAnalyzer &analyzer);

    void report_err_expr_category(const sir::Expr &expr, sir::ExprCategory expected);
    void report_err_symbol_not_found(const sir::IdentExpr &ident_expr);
    void report_err_symbol_not_found(const sir::Ident &ident);
    void report_err_symbol_not_found_in(const sir::Ident &member_ident, const std::string &base_name);
    void report_err_module_not_found(const sir::Ident &ident);
    void report_err_redefinition(const sir::Ident &ident, const sir::Symbol &prev_def);
    void report_err_type_mismatch(const sir::Expr &value, const sir::Expr &expected, const sir::Expr &actual);
    void report_err_cannot_coerce(const sir::Expr &expr, const sir::Expr &expected_type);
    void report_err_cannot_coerce(const sir::IntLiteral &int_literal, const sir::Expr &expected_type);
    void report_err_cannot_coerce(const sir::FPLiteral &fp_literal, const sir::Expr &expected_type);
    void report_err_cannot_coerce(const sir::NullLiteral &null_literal, const sir::Expr &expected_type);
    void report_err_cannot_coerce(const sir::NoneLiteral &none_literal, const sir::Expr &expected_type);
    void report_err_cannot_coerce(const sir::ArrayLiteral &array_literal, const sir::Expr &expected_type);
    void report_err_cannot_coerce(const sir::StringLiteral &string_literal, const sir::Expr &expected_type);
    void report_err_cannot_coerce(const sir::StructLiteral &struct_literal, const sir::Expr &expected_type);
    void report_err_cannot_coerce(const sir::MapLiteral &map_literal, const sir::Expr &expected_type);

    void report_err_cannot_coerce_result(
        const sir::Expr &value,
        sir::Specialization<sir::StructDef> &result_specialization
    );

    void report_err_cannot_infer_type(const sir::NoneLiteral &none_literal);
    void report_err_cannot_infer_type(const sir::UndefinedLiteral &undefined_literal);
    void report_err_cannot_infer_type(const sir::ArrayLiteral &array_literal);
    void report_err_cannot_infer_type(const sir::StructLiteral &struct_literal);
    void report_err_operator_overload_not_found(const sir::BinaryExpr &binary_expr);
    void report_err_operator_overload_not_found(const sir::UnaryExpr &unary_expr);
    void report_err_operator_overload_not_found(const sir::StarExpr &star_expr);
    void report_err_operator_overload_not_found(const sir::BracketExpr &bracket_expr);
    void report_err_cannot_cast(const sir::CastExpr &cast_expr);
    void report_err_cannot_call(const sir::Expr &expr);
    void report_err_cannot_deref(const sir::Expr &expr);
    void report_err_cannot_iter(const sir::Expr &expr);
    void report_err_cannot_iter_struct(const sir::Expr &expr, bool by_ref);
    void report_err_iter_no_next(const sir::Expr &expr, const sir::FuncDef &iter_func_def, bool by_ref);
    void report_err_expected_integer(const sir::Expr &expr);
    void report_err_expected_bool(const sir::Expr &expr);
    void report_err_expected_generic_or_indexable(sir::Expr &expr);
    void report_err_unexpected_arg_count(sir::CallExpr &call_expr, unsigned expected_count, sir::FuncDef *func_def);
    void report_err_no_members(const sir::DotExpr &dot_expr);
    void report_err_no_field(const sir::Ident &field_ident, const sir::StructDef &struct_def);
    void report_err_no_field(const sir::Ident &field_ident, const sir::UnionCase &union_case);
    void report_err_no_field(const sir::Ident &field_ident, sir::TupleExpr &tuple_expr);
    void report_err_missing_field(const sir::StructLiteral &struct_literal, const sir::StructField &field);

    void report_err_duplicate_field(
        const sir::StructLiteral &struct_literal,
        const sir::StructLiteralEntry &entry,
        const sir::StructLiteralEntry &previous_entry
    );

    void report_err_struct_overlapping_not_one_field(const sir::StructLiteral &struct_literal);
    void report_err_no_method(const sir::Ident &method_ident, const sir::StructDef &struct_def);
    void report_err_no_matching_overload(const sir::Expr &expr, sir::OverloadSet &overload_set);
    void report_err_continue_outside_loop(const sir::ContinueStmt &continue_stmt);
    void report_err_break_outside_loop(const sir::BreakStmt &break_stmt);
    void report_err_unexpected_array_length_type(const sir::Expr &expr);
    void report_err_negative_array_length(const sir::Expr &expr, LargeInt value);
    void report_err_unexpected_array_length(const sir::ArrayLiteral &array_literal, unsigned expected_count);
    void report_err_cannot_negate(const sir::UnaryExpr &unary_expr);
    void report_err_cannot_negate_unsigned(const sir::UnaryExpr &unary_expr);
    void report_err_expected_index(const sir::BracketExpr &bracket_expr);
    void report_err_too_many_indices(const sir::BracketExpr &bracket_expr);
    void report_err_unexpected_generic_arg_count(sir::BracketExpr &bracket_expr, sir::FuncDef &func_def);
    void report_err_unexpected_generic_arg_count(sir::BracketExpr &bracket_expr, sir::StructDef &struct_def);
    void report_err_too_few_args_to_infer_generic_args(const sir::Expr &expr);
    void report_err_cannot_infer_generic_arg(const sir::Expr &expr, const sir::GenericParam &generic_param);

    void report_err_generic_arg_inference_conflict(
        const sir::Expr &expr,
        const sir::GenericParam &generic_param,
        const sir::Expr &first_source,
        const sir::Expr &second_source
    );

    void report_err_cannot_use_in_try(const sir::Expr &expr);
    void report_err_try_no_error_field(const sir::TryExceptBranch &branch);
    void report_err_compile_time_unknown(const sir::Expr &value);

    void report_err_expected_proto(const sir::Expr &expr);
    void report_err_impl_missing_func(const sir::StructDef &struct_def, const sir::ProtoFuncDecl &func_decl);
    void report_err_impl_type_mismatch(sir::FuncDef &func_def, sir::ProtoFuncDecl &func_decl);

    void report_err_self_not_allowed(const sir::Param &param);
    void report_err_self_not_first(const sir::Param &param);
    void report_err_case_outside_union(const sir::UnionCase &union_case);
    void report_err_func_decl_outside_proto(const sir::FuncDecl &func_decl);
    void report_err_struct_overlapping_no_fields(const sir::StructDef &struct_def);
    void report_err_invalid_global_value(const sir::Expr &value);

    void report_err_return_missing_value(const sir::ReturnStmt &return_stmt, const sir::Expr &expected_type);
    void report_err_does_not_return(const sir::Ident &func_ident);
    void report_err_does_not_always_return(const sir::Ident &func_ident);
    void report_err_pointer_to_local_escapes(const sir::Stmt &stmt, const sir::UnaryExpr &ref_expr);

    void report_err_use_after_move(const sir::Expr &use, const sir::Expr &move, bool partial, bool conditional);
    void report_err_move_out_pointer(const sir::Expr &move);
    void report_err_move_out_deinit(const sir::Expr &move);
    void report_err_move_in_loop(const sir::Expr &move);

    void report_err_invalid_meta_field(const sir::MetaFieldExpr &meta_field_expr);
    void report_err_invalid_meta_method(const sir::MetaCallExpr &meta_call_expr);

    void report_warn_unreachable_code(const sir::Stmt &stmt);

private:
    template <typename... FormatArgs>
    ReportBuilder build_error(std::string_view format_str, ASTNode *node, FormatArgs... format_args);

    template <typename... FormatArgs>
    ReportBuilder build_warning(std::string_view format_str, ASTNode *node, FormatArgs... format_args);

    template <typename... FormatArgs>
    void report_error(std::string_view format_str, ASTNode *node, FormatArgs... format_args);

    template <typename... FormatArgs>
    void report_warning(std::string_view format_str, ASTNode *node, FormatArgs... format_args);

    void report_err_operator_overload_not_found(
        ASTNode *ast_node,
        sir::Expr type,
        std::string_view operator_name,
        std::string_view impl_name
    );

    void report_err_unexpected_generic_arg_count(
        sir::BracketExpr &bracket_expr,
        std::vector<sir::GenericParam> &generic_params,
        sir::Ident &decl_ident,
        std::string_view decl_kind
    );
};

} // namespace sema

} // namespace lang

} // namespace banjo

#endif
