#ifndef BANJO_SEMA_EXPR_PROPERTY_ANALYZER_H
#define BANJO_SEMA_EXPR_PROPERTY_ANALYZER_H

#include "banjo/sir/sir.hpp"

namespace banjo {
namespace lang {
namespace sema {

enum class Mutability {
    MUTABLE,
    IMMUTABLE_REF,
};

struct ExprProperties {
    sir::Expr base_value = nullptr;
    Mutability mutability;
    bool is_temporary;
};

class ExprPropertyAnalyzer {

public:
    ExprPropertyAnalyzer();
    ExprProperties analyze(sir::Expr expr);

private:
    ExprProperties analyze_unary_expr(sir::UnaryExpr &unary_expr);
    ExprProperties analyze_index_expr(sir::IndexExpr &index_expr);
    ExprProperties analyze_field_expr(sir::FieldExpr &field_expr);
};

} // namespace sema
} // namespace lang
} // namespace banjo

#endif
