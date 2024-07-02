#ifndef IR_BUILDER_BLOCK_IR_BUILDER_H
#define IR_BUILDER_BLOCK_IR_BUILDER_H

#include "banjo/ir_builder/deinit_ir_builder.hpp"
#include "banjo/ir_builder/ir_builder.hpp"
#include "banjo/symbol/variable.hpp"

namespace banjo {

namespace ir_builder {

class BlockIRBuilder : public IRBuilder {

private:
    ir::BasicBlockIter exit = nullptr;
    bool local_vars_allocated = false;
    DeinitIRBuilder deinit_builder;

public:
    BlockIRBuilder(IRBuilderContext &context, lang::ASTNode *node);
    ir::BasicBlockIter create_exit();
    void build();
    void alloc_local_vars();
    void build_children();
    void build_exit();
};

} // namespace ir_builder

} // namespace banjo

#endif
