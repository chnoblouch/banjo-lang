#include "report_generator.hpp"

#include "banjo/ast/ast_module.hpp"
#include "banjo/ast/ast_node.hpp"
#include "banjo/sema2/semantic_analyzer.hpp"
#include "banjo/sir/sir.hpp"
#include "banjo/utils/macros.hpp"
#include "magic_methods.hpp"

namespace banjo {

namespace lang {

namespace sema {

ReportBuilder::ReportBuilder(SemanticAnalyzer &analyzer, Report::Type type)
  : analyzer(analyzer),
    partial_report(type) {}

template <typename... FormatArgs>
ReportBuilder &ReportBuilder::set_message(std::string_view format_str, ASTNode *node, FormatArgs... format_args) {
    SourceLocation location{find_mod_path(node), node->get_range()};
    partial_report.set_message({location, format_str, format_args...});
    return *this;
}

template <typename... FormatArgs>
ReportBuilder &ReportBuilder::add_note(std::string_view format_str, ASTNode *node, FormatArgs... format_args) {
    SourceLocation location{find_mod_path(node), node->get_range()};
    partial_report.add_note({location, format_str, format_args...});
    return *this;
}

void ReportBuilder::report() {
    analyzer.report_manager.insert(partial_report);
}

const ModulePath &ReportBuilder::find_mod_path(ASTNode *node) {
    while (node->get_type() != AST_MODULE) {
        node = node->get_parent();
    }

    return node->as<ASTModule>()->get_path();
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
    report_error("cannot find '$'", ident_expr.ast_node, ident_expr.value);
}

void ReportGenerator::report_err_symbol_not_found(const sir::Ident &ident) {
    report_error("cannot find '$'", ident.ast_node, ident.value);
}

void ReportGenerator::report_err_symbol_not_found_in(const sir::Ident &member_ident, const std::string &base_name) {
    report_error("cannot find '$' in '$'", member_ident.ast_node, member_ident.value, base_name);
}

void ReportGenerator::report_err_module_not_found(const sir::Ident &ident) {
    report_error("cannot find module '$'", ident.ast_node, ident.value);
}

void ReportGenerator::report_err_redefinition(const sir::Ident &ident, const sir::Symbol &prev_def) {
    build_error("redefinition of '$'", ident.ast_node, ident.value)
        .add_note("previously defined here", prev_def.get_ident().ast_node)
        .report();
}

void ReportGenerator::report_err_type_mismatch(
    const sir::Expr &value,
    const sir::Expr &expected,
    const sir::Expr &actual
) {
    report_error("type mismatch (expected '$', got '$')", value.get_ast_node(), expected, actual);
}

void ReportGenerator::report_err_cant_coerce_int_literal(
    const sir::IntLiteral &int_literal,
    const sir::Expr &expected_type
) {
    report_error("cannot coerce int literal to type '$'", int_literal.ast_node, expected_type);
}

void ReportGenerator::report_err_cant_coerce_fp_literal(
    const sir::FPLiteral &fp_literal,
    const sir::Expr &expected_type
) {
    report_error("cannot coerce float literal to type '$'", fp_literal.ast_node, expected_type);
}

void ReportGenerator::report_err_cant_coerce_null_literal(
    const sir::NullLiteral &null_literal,
    const sir::Expr &expected_type
) {
    report_error("cannot coerce null to type '$'", null_literal.ast_node, expected_type);
}

void ReportGenerator::report_err_cant_coerce_string_literal(
    const sir::StringLiteral &string_literal,
    const sir::Expr &expected_type
) {
    report_error("cannot coerce string literal to type '$'", string_literal.ast_node, expected_type);
}

void ReportGenerator::report_err_operator_overload_not_found(const sir::BinaryExpr &binary_expr) {
    std::string_view operator_name;

    switch (binary_expr.op) {
        case sir::BinaryOp::ADD: operator_name = "+"; break;
        case sir::BinaryOp::SUB: operator_name = "-"; break;
        case sir::BinaryOp::MUL: operator_name = "*"; break;
        case sir::BinaryOp::DIV: operator_name = "/"; break;
        case sir::BinaryOp::MOD: operator_name = "%"; break;
        case sir::BinaryOp::BIT_AND: operator_name = "&"; break;
        case sir::BinaryOp::BIT_OR: operator_name = "|"; break;
        case sir::BinaryOp::BIT_XOR: operator_name = "^"; break;
        case sir::BinaryOp::SHL: operator_name = "<<"; break;
        case sir::BinaryOp::SHR: operator_name = ">>"; break;
        case sir::BinaryOp::EQ: operator_name = "=="; break;
        case sir::BinaryOp::NE: operator_name = "!="; break;
        case sir::BinaryOp::GT: operator_name = ">"; break;
        case sir::BinaryOp::LT: operator_name = "<"; break;
        case sir::BinaryOp::GE: operator_name = ">="; break;
        case sir::BinaryOp::LE: operator_name = "<="; break;
        default: ASSERT_UNREACHABLE;
    }

    std::string_view impl_name = MagicMethods::look_up(binary_expr.op);
    report_err_operator_overload_not_found(binary_expr.ast_node, binary_expr.lhs.get_type(), operator_name, impl_name);
}

void ReportGenerator::report_err_operator_overload_not_found(const sir::UnaryExpr &unary_expr) {
    ASSUME(unary_expr.op == sir::UnaryOp::NEG);
    std::string_view impl_name = MagicMethods::look_up(unary_expr.op);
    report_err_operator_overload_not_found(unary_expr.ast_node, unary_expr.value.get_type(), "-", impl_name);
}

void ReportGenerator::report_err_operator_overload_not_found(const sir::StarExpr &star_expr) {
    std::string_view impl_name = MagicMethods::look_up(sir::UnaryOp::DEREF);
    report_err_operator_overload_not_found(star_expr.ast_node, star_expr.value.get_type(), "*", impl_name);
}

void ReportGenerator::report_err_operator_overload_not_found(const sir::BracketExpr &bracket_expr) {
    report_err_operator_overload_not_found(
        bracket_expr.ast_node,
        bracket_expr.lhs.get_type(),
        "[]",
        MagicMethods::OP_INDEX
    );
}

void ReportGenerator::report_cannot_call(const sir::Expr &expr) {
    report_error("cannot call value with type '$'", expr.get_ast_node(), expr.get_type());
}

void ReportGenerator::report_err_operator_overload_not_found(
    ASTNode *ast_node,
    sir::Expr type,
    std::string_view operator_name,
    std::string_view impl_name
) {
    ReportBuilder builder =
        build_error("no implementation of operator '$' for type '$'", ast_node, operator_name, type);

    if (auto struct_def = type.match_symbol<sir::StructDef>()) {
        builder.add_note("implement '$' for this type to support this operator", struct_def->ident.ast_node, impl_name);
    }

    builder.report();
}

} // namespace sema

} // namespace lang

} // namespace banjo
