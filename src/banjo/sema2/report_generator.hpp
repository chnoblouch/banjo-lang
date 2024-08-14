#ifndef REPORT_GENERATOR_H
#define REPORT_GENERATOR_H

#include "banjo/reports/report_manager.hpp"
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

    void report();
};

class ReportGenerator {

private:
    SemanticAnalyzer &analyzer;

public:
    ReportGenerator(SemanticAnalyzer &analyzer);

    void report_err_symbol_not_found(const sir::IdentExpr &ident_expr);
    void report_err_symbol_not_found_in(const sir::Ident &member_ident, const std::string &base_name);
    void report_err_module_not_found(const sir::Ident &ident);
    void report_err_redefinition(const sir::Ident &ident, const sir::Symbol &prev_def);
    void report_err_type_mismatch(const sir::Expr &value, const sir::Expr &expected, const sir::Expr &actual);
    void report_err_cant_coerce_int_literal(const sir::IntLiteral &int_literal, const sir::Expr &expected_type);
    void report_err_cant_coerce_fp_literal(const sir::FPLiteral &fp_literal, const sir::Expr &expected_type);
    void report_err_cant_coerce_null_literal(const sir::NullLiteral &null_literal, const sir::Expr &expected_type);
   
    void report_err_cant_coerce_string_literal(
        const sir::StringLiteral &string_literal,
        const sir::Expr &expected_type
    );

private:
    template <typename... FormatArgs>
    ReportBuilder build_error(std::string_view format_str, ASTNode *node, FormatArgs... format_args);

    template <typename... FormatArgs>
    void report_error(std::string_view format_str, ASTNode *node, FormatArgs... format_args);
};

} // namespace sema

} // namespace lang

} // namespace banjo

#endif
