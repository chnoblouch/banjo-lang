#ifndef IR_BUILDER_TRY_IR_BUILDER_H
#define IR_BUILDER_TRY_IR_BUILDER_H

#include "ir/basic_block.hpp"
#include "ir_builder/ir_builder.hpp"
#include "ir_builder/storage.hpp"

namespace ir_builder {

class TryIRBuilder : public IRBuilder {

private:
    std::string id_str;
    ir::BasicBlockIter success_block;
    ir::BasicBlockIter exit_block;
    StoredValue value;

public:
    TryIRBuilder(IRBuilderContext &context, lang::ASTNode *node) : IRBuilder(context, node) {}
    void build();

private:
    void build_case(unsigned index);
    void build_success_case(lang::ASTNode *node, ir::BasicBlockIter next_block);
    void build_error_case(lang::ASTNode *node);
    void build_else_case(lang::ASTNode *node);
};

} // namespace ir_builder

#endif
