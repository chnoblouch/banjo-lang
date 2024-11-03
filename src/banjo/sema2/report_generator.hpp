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
    void report_err_cannot_infer_type(const sir::NoneLiteral &none_literal);
    void report_err_cannot_infer_type(const sir::UndefinedLiteral &undefined_literal);
    void report_err_cannot_infer_type(const sir::ArrayLiteral &array_literal);
    void report_err_cannot_infer_type(const sir::StructLiteral &struct_literal);
    void report_err_operator_overload_not_found(const sir::BinaryExpr &binary_expr);
    void report_err_operator_overload_not_found(const sir::UnaryExpr &unary_expr);
    void report_err_operator_overload_not_found(const sir::StarExpr &star_expr);
    void report_err_operator_overload_not_found(const sir::BracketExpr &bracket_expr);
    void report_err_cannot_call(const sir::Expr &expr);
    void report_err_cannot_deref(const sir::Expr &expr);
    void report_err_no_members(const sir::DotExpr &dot_expr);
    void report_err_no_field(const sir::Ident &field_ident, const sir::StructDef &struct_def);
    void report_err_no_method(const sir::Ident &method_ident, const sir::StructDef &struct_def);
    void report_err_unexpected_array_length(const sir::ArrayLiteral &array_literal, unsigned expected_count);
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
    void report_err_compile_time_unknown(const sir::Expr &range);
    void report_err_meta_for_cannot_iter(const sir::Expr &range);

    void report_err_expected_proto(const sir::Expr &expr);
    void report_err_impl_missing_func(const sir::StructDef &struct_def, const sir::ProtoFuncDecl &func_decl);
    void report_err_impl_type_mismatch(sir::FuncDef &func_def, sir::ProtoFuncDecl &func_decl);

    void report_err_use_after_move(const sir::Expr &use, const sir::Expr &move, bool partial, bool conditional);
    void report_err_move_out_pointer(const sir::Expr &move);
    void report_err_move_out_deinit(const sir::Expr &move);

private:
    template <typename... FormatArgs>
    ReportBuilder build_error(std::string_view format_str, ASTNode *node, FormatArgs... format_args);

    template <typename... FormatArgs>
    void report_error(std::string_view format_str, ASTNode *node, FormatArgs... format_args);

    void report_err_operator_overload_not_found(
        ASTNode *ast_node,
        sir::Expr type,
        std::string_view operator_name,
        std::string_view impl_name
    );
};

} // namespace sema

} // namespace lang

} // namespace banjo

#endif
