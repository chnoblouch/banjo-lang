#ifndef BANJO_SEMA_MUTABILITY_CHECKER_H
#define BANJO_SEMA_MUTABILITY_CHECKER_H

#include "banjo/sir/sir.hpp"

namespace banjo {
namespace lang {
namespace sema {

class MutabilityChecker {

public:
    enum class Kind {
        MUTABLE,
        IMMUTABLE_REF,
    };

    struct Result {
        Kind kind;
        sir::Expr immut_expr = nullptr;
    };

public:
    MutabilityChecker();
    Result check(sir::Expr expr);

private:
    Result check_unary_expr(sir::UnaryExpr &unary_expr);
    Result check_index_expr(sir::IndexExpr &index_expr);
    Result check_field_expr(sir::FieldExpr &field_expr);
};

} // namespace sema
} // namespace lang
} // namespace banjo

#endif
