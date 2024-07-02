#ifndef DEINIT_IR_BUILDER_H
#define DEINIT_IR_BUILDER_H

#include "ir_builder/ir_builder.hpp"
#include "symbol/variable.hpp"

namespace banjo {

namespace ir_builder {

class DeinitIRBuilder : public IRBuilder {

public:
    DeinitIRBuilder(IRBuilderContext &context, lang::ASTNode *block_node) : IRBuilder(context, block_node) {}
    void build();
    void build_exit();

private:
    void build_cond_deinit_call(lang::DeinitInfo *info);
    void build_deinit_call(lang::DeinitInfo *info);
};

} // namespace ir_builder

} // namespace banjo

#endif
