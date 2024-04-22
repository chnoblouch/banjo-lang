#ifndef IR_BUILDER_CLOSURE_IR_BUILDER_H
#define IR_BUILDER_CLOSURE_IR_BUILDER_H

#include "ir_builder/ir_builder.hpp"
#include "ir_builder/storage.hpp"

namespace ir_builder {

class ClosureIRBuilder : public IRBuilder {

private:
    Closure closure;

public:
    ClosureIRBuilder(IRBuilderContext &context, lang::ASTNode *node) : IRBuilder(context, node) {}
    StoredValue build(StorageHints hints);

private:
    ir::VirtualRegister create_data();
};

} // namespace ir_builder

#endif
