#include "const_evaluator.hpp"

#include "ast/expr.hpp"
#include <cassert>

namespace banjo {

namespace lang {

LargeInt ConstEvaluator::eval_int(ASTNode *node) {
    if (node->get_type() == AST_INT_LITERAL) {
        return node->as<IntLiteral>()->get_value();
    } else if (node->get_type() == AST_OPERATOR_NEG) {
        return -eval_int(node->get_child());
    } else {
        assert("value is not const");
        return 0;
    }
}

} // namespace lang

} // namespace banjo
