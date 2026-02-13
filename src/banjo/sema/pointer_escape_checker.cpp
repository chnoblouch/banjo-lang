#include "pointer_escape_checker.hpp"
#include "banjo/sir/sir.hpp"

namespace banjo {
namespace lang {
namespace sema {

PointerEscapeChecker::PointerEscapeChecker(SemanticAnalyzer &analyzer) : analyzer(analyzer) {}

void PointerEscapeChecker::check_return_stmt(sir::ReturnStmt &return_stmt) {
    if (!return_stmt.value) {
        return;
    }

    check_escaping_value(&return_stmt, return_stmt.value);
}

void PointerEscapeChecker::check_escaping_value(sir::Stmt stmt, sir::Expr value) {
    if (auto unary_expr = value.match<sir::UnaryExpr>()) {
        if (unary_expr->op != sir::UnaryOp::ADDR) {
            return;
        }

        if (unary_expr->value.is_symbol<sir::Local>()) {
            analyzer.report_generator.report_err_pointer_to_local_escapes(stmt, *unary_expr);
        }
    } else if (auto struct_literal = value.match<sir::StructLiteral>()) {
        for (const sir::StructLiteralEntry &entry : struct_literal->entries) {
            check_escaping_value(stmt, entry.value);
        }
    } else if (auto tuple_literal = value.match<sir::TupleExpr>()) {
        for (const sir::Expr &tuple_value : tuple_literal->exprs) {
            check_escaping_value(stmt, tuple_value);
        }
    }
}

} // namespace sema
} // namespace lang
} // namespace banjo
