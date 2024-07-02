#ifndef CONST_EVALUATOR_H
#define CONST_EVALUATOR_H

#include "banjo/ast/ast_node.hpp"
#include "banjo/utils/large_int.hpp"

namespace banjo {

namespace lang {

class ConstEvaluator {

public:
    LargeInt eval_int(ASTNode *node);
};

} // namespace lang

} // namespace banjo

#endif
