#ifndef IR_BUILDER_CLOSURE_IR_BUILDER_H
#define IR_BUILDER_CLOSURE_IR_BUILDER_H

#include "banjo/ir_builder/ir_builder.hpp"
#include "banjo/ir_builder/storage.hpp"

namespace banjo {

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

} // namespace banjo

#endif
