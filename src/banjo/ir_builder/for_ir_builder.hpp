#ifndef IR_BUILDER_FOR_IR_BUILDER_H
#define IR_BUILDER_FOR_IR_BUILDER_H

#include "ir_builder/ir_builder.hpp"
#include "symbol/local_variable.hpp"

namespace banjo {

namespace ir_builder {

class ForIRBuilder : public IRBuilder {

private:
    lang::LocalVariable *var;
    ir::BasicBlockIter entry;
    ir::BasicBlockIter block;
    ir::BasicBlockIter exit;

public:
    ForIRBuilder(IRBuilderContext &context, lang::ASTNode *node) : IRBuilder(context, node) {}
    void build();

private:
    void build_range_for();
    void build_iter_for();
};

} // namespace ir_builder

} // namespace banjo

#endif
