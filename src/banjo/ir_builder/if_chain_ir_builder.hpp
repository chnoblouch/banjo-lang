#ifndef IR_BUILDER_IF_CHAIN_IR_BUILDER_H
#define IR_BUILDER_IF_CHAIN_IR_BUILDER_H

#include "banjo/ir_builder/ir_builder.hpp"

namespace banjo {

namespace ir_builder {

class IfChainIRBuilder : public IRBuilder {

private:
    int branch_id;
    ir::BasicBlockIter end_block;
    ir::BasicBlockIter else_block;

public:
    IfChainIRBuilder(IRBuilderContext &context, lang::ASTNode *node) : IRBuilder(context, node) {}
    void build();

private:
    void build_branches();

    ir::BasicBlockIter create_next_block(unsigned cur_index);
    std::string get_condition_label(unsigned index);
    std::string get_then_label(unsigned index);
};

} // namespace ir_builder

} // namespace banjo

#endif
