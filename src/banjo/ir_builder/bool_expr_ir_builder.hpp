#ifndef IR_BUILDER_BOOL_EXPR_BUILDER_H
#define IR_BUILDER_BOOL_EXPR_BUILDER_H

#include "ir/comparison.hpp"
#include "ir_builder/ir_builder.hpp"
#include <string>

namespace banjo {

namespace ir_builder {

class BoolExprIRBuilder : public IRBuilder {

private:
    ir::BasicBlockIter true_block;
    ir::BasicBlockIter false_block;

public:
    BoolExprIRBuilder(
        IRBuilderContext &context,
        lang::ASTNode *node,
        ir::BasicBlockIter true_block,
        ir::BasicBlockIter false_block
    )
      : IRBuilder(context, node),
        true_block(true_block),
        false_block(false_block) {}

    void build();

private:
    void build_and();
    void build_or();
    void build_not();
    void build_comparison(ir::Comparison int_comparison, ir::Comparison fp_comparison);
    void build_bool_eval();

    void build_bool_eval(ir::Value value);
    ir::Opcode get_cmp_opcode(bool is_fp);
};

} // namespace ir_builder

} // namespace banjo

#endif
