#ifndef IR_BUILDER_STRUCT_DEF_IR_BUILDER_H
#define IR_BUILDER_STRUCT_DEF_IR_BUILDER_H

#include "banjo/ir_builder/ir_builder.hpp"

namespace banjo {

namespace ir_builder {

class StructDefIRBuilder : public IRBuilder {

public:
    StructDefIRBuilder(IRBuilderContext &context, lang::ASTNode *node) : IRBuilder(context, node) {}
    void build();
};

} // namespace ir_builder

} // namespace banjo

#endif
