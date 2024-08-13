#include "report_generator.hpp"

#include "banjo/sema2/semantic_analyzer.hpp"

namespace banjo {

namespace lang {

namespace sema {

ReportBuilder::ReportBuilder(SemanticAnalyzer &analyzer, Report::Type type)
  : analyzer(analyzer),
    partial_report(type) {}

template <typename... FormatArgs>
ReportBuilder &ReportBuilder::set_message(std::string_view format_str, ASTNode *node, FormatArgs... format_args) {
    SourceLocation location{analyzer.cur_sir_mod->path, node->get_range()};
    partial_report.set_message({location, format_str, format_args...});
    return *this;
}

template <typename... FormatArgs>
ReportBuilder &ReportBuilder::add_note(std::string_view format_str, ASTNode *node, FormatArgs... format_args) {
    SourceLocation location{analyzer.cur_sir_mod->path, node->get_range()};
    partial_report.add_note({location, format_str, format_args...});
    return *this;
}

void ReportBuilder::report() {
    analyzer.report_manager.insert(partial_report);
}

ReportGenerator::ReportGenerator(SemanticAnalyzer &analyzer) : analyzer(analyzer) {}

template <typename... FormatArgs>
ReportBuilder ReportGenerator::build_error(std::string_view format_str, ASTNode *node, FormatArgs... format_args) {
    return ReportBuilder(analyzer, Report::Type::ERROR).set_message(format_str, node, format_args...);
}

template <typename... FormatArgs>
void ReportGenerator::report_error(std::string_view format_str, ASTNode *node, FormatArgs... format_args) {
    build_error(format_str, node, format_args...).report();
}

void ReportGenerator::report_err_symbol_not_found(const sir::IdentExpr &ident_expr) {
    report_error("cannot find '%'", ident_expr.ast_node, ident_expr.value);
}

void ReportGenerator::report_err_symbol_not_found_in(const sir::Ident &member_ident, const std::string &base_name) {
    report_error("cannot find '%' in '%'", member_ident.ast_node, member_ident.value, base_name);
}

void ReportGenerator::report_err_module_not_found(const sir::Ident &ident) {
    report_error("cannot find module '%'", ident.ast_node, ident.value);
}

void ReportGenerator::report_err_redefinition(const sir::Ident &ident, const sir::Symbol &prev_def) {
    build_error("redefinition of '%'", ident.ast_node, ident.value)
        .add_note("previously defined here", prev_def.get_ident().ast_node)
        .report();
}

void ReportGenerator::report_err_type_mismatch(
    const sir::Expr &value,
    const sir::Expr &expected,
    const sir::Expr &actual
) {
    report_error("type mismatch (expected '%', got '%')", value.get_ast_node(), expected, actual);
}

void ReportGenerator::report_err_cant_coerce_int_literal(
    const sir::IntLiteral &int_literal,
    const sir::Expr &expected_type
) {
    report_error("cannot coerce int literal to type '%'", int_literal.ast_node, expected_type);
}

void ReportGenerator::report_err_cant_coerce_fp_literal(
    const sir::FPLiteral &fp_literal,
    const sir::Expr &expected_type
) {
    report_error("cannot coerce float literal to type '%'", fp_literal.ast_node, expected_type);
}

} // namespace sema

} // namespace lang

} // namespace banjo
