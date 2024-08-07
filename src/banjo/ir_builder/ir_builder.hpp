#ifndef IR_BUILDER_IR_BUILDER_H
#define IR_BUILDER_IR_BUILDER_H

#include "banjo/ast/ast_node.hpp"
#include "banjo/ir_builder/ir_builder_context.hpp"

namespace banjo {

namespace ir_builder {

class IRBuilder {

protected:
    IRBuilderContext &context;
    lang::ASTNode *node;

public:
    IRBuilder(IRBuilderContext &context, lang::ASTNode *node) : context(context), node(node) {}

protected:
    unsigned get_size(const ir::Type &type);
    bool is_pass_by_ref(const ir::Type &type);
    bool is_return_by_ref(const ir::Type &type);
};

} // namespace ir_builder

} // namespace banjo

#endif
