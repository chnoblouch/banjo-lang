#include "mutability_checker.hpp"

#include "banjo/sir/sir.hpp"

namespace banjo {
namespace lang {
namespace sema {

MutabilityChecker::MutabilityChecker() {}

MutabilityChecker::Result MutabilityChecker::check(sir::Expr expr) {
    if (auto unary_expr = expr.match<sir::UnaryExpr>()) {
        return check_unary_expr(*unary_expr);
    } else if (auto index_expr = expr.match<sir::IndexExpr>()) {
        return check_index_expr(*index_expr);
    } else if (auto field_expr = expr.match<sir::FieldExpr>()) {
        return check_field_expr(*field_expr);
    } else {
        return Result{.kind = Kind::MUTABLE};
    }
}

MutabilityChecker::Result MutabilityChecker::check_index_expr(sir::IndexExpr &index_expr) {
    return check(index_expr.base);
}

MutabilityChecker::Result MutabilityChecker::check_unary_expr(sir::UnaryExpr &unary_expr) {
    if (unary_expr.op != sir::UnaryOp::DEREF) {
        return Result{.kind = Kind::MUTABLE};
    }

    if (auto reference_type = unary_expr.value.get_type().match<sir::ReferenceType>()) {
        if (reference_type->mut) {
            return Result{.kind = Kind::MUTABLE};
        } else {
            return Result{.kind = Kind::IMMUTABLE_REF, .immut_expr = unary_expr.value};
        }
    } else {
        return Result{.kind = Kind::MUTABLE};
    }
}

MutabilityChecker::Result MutabilityChecker::check_field_expr(sir::FieldExpr &field_expr) {
    return check(field_expr.base);
}

} // namespace sema
} // namespace lang
} // namespace banjo
