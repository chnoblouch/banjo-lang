#ifndef IR_BUILDER_VAR_DECL_IR_BUILDER_H
#define IR_BUILDER_VAR_DECL_IR_BUILDER_H

#include "ir_builder/ir_builder.hpp"

namespace banjo {

namespace ir_builder {

class VarDeclIRBuilder : public IRBuilder {

public:
    VarDeclIRBuilder(IRBuilderContext &context, lang::ASTNode *node) : IRBuilder(context, node) {}
    void build();
};

} // namespace ir_builder

} // namespace banjo

#endif
