#include "expr_property_analyzer.hpp"

#include "banjo/sir/sir.hpp"

namespace banjo {
namespace lang {
namespace sema {

ExprPropertyAnalyzer::ExprPropertyAnalyzer() {}

ExprProperties ExprPropertyAnalyzer::analyze(sir::Expr expr) {
    if (auto unary_expr = expr.match<sir::UnaryExpr>()) {
        return analyze_unary_expr(*unary_expr);
    } else if (auto index_expr = expr.match<sir::IndexExpr>()) {
        return analyze_index_expr(*index_expr);
    } else if (auto field_expr = expr.match<sir::FieldExpr>()) {
        return analyze_field_expr(*field_expr);
    } else if (expr.is<sir::SymbolExpr>()) {
        return ExprProperties{
            .base_value = expr,
            .mutability = Mutability::MUTABLE,
            .is_temporary = false,
        };
    } else {
        return ExprProperties{
            .base_value = expr,
            .mutability = Mutability::MUTABLE,
            .is_temporary = true,
        };
    }
}

ExprProperties ExprPropertyAnalyzer::analyze_index_expr(sir::IndexExpr &index_expr) {
    return analyze(index_expr.base);
}

ExprProperties ExprPropertyAnalyzer::analyze_unary_expr(sir::UnaryExpr &unary_expr) {
    if (unary_expr.op != sir::UnaryOp::DEREF) {
        return ExprProperties{
            .base_value = &unary_expr,
            .mutability = Mutability::MUTABLE,
            .is_temporary = true,
        };
    }

    if (auto reference_type = unary_expr.value.get_type().match<sir::ReferenceType>()) {
        if (reference_type->mut) {
            return ExprProperties{
                .base_value = unary_expr.value,
                .mutability = Mutability::MUTABLE,
                .is_temporary = false,
            };
        } else {
            return ExprProperties{
                .base_value = unary_expr.value,
                .mutability = Mutability::IMMUTABLE_REF,
                .is_temporary = false,
            };
        }
    } else {
        return ExprProperties{
            .base_value = &unary_expr,
            .mutability = Mutability::MUTABLE,
            .is_temporary = true,
        };
    }
}

ExprProperties ExprPropertyAnalyzer::analyze_field_expr(sir::FieldExpr &field_expr) {
    return analyze(field_expr.base);
}

} // namespace sema
} // namespace lang
} // namespace banjo
