#ifndef IR_BUILDER_TYPE_INFER_VAR_DECL_IR_BUILDER_H
#define IR_BUILDER_TYPE_INFER_VAR_DECL_IR_BUILDER_H

#include "banjo/ir_builder/ir_builder.hpp"

namespace banjo {

namespace ir_builder {

class TypeInferVarDeclIRBuilder : public IRBuilder {

public:
    TypeInferVarDeclIRBuilder(IRBuilderContext &context, lang::ASTNode *node) : IRBuilder(context, node) {}
    void build();
};

} // namespace ir_builder

} // namespace banjo

#endif
