#ifndef IR_BUILDER_RETURN_IR_BUILDER_H
#define IR_BUILDER_RETURN_IR_BUILDER_H

#include "ir_builder/ir_builder.hpp"
#include <optional>

namespace ir_builder {

class ReturnIRBuilder : public IRBuilder {

public:
    ReturnIRBuilder(IRBuilderContext &context, lang::ASTNode *node) : IRBuilder(context, node) {}

    void build();

private:
    void build_return_value();
};

} // namespace ir_builder

#endif
