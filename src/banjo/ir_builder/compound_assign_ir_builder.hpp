#ifndef IR_BUILDER_COMPOUND_ASSIGN_IR_BUILDER_H
#define IR_BUILDER_COMPOUND_ASSIGN_IR_BUILDER_H

#include "ir_builder/ir_builder.hpp"

namespace ir_builder {

class CompoundAssignIRBuilder : public IRBuilder {

public:
    CompoundAssignIRBuilder(IRBuilderContext &context, lang::ASTNode *node) : IRBuilder(context, node) {}
    void build();

private:
    ir::Opcode get_opcode(lang::ASTNodeType node_type, const ir::Type &type);
    ir::Opcode get_int_opcode(lang::ASTNodeType node_type);
    ir::Opcode get_fp_opcode(lang::ASTNodeType node_type);
    ir::Opcode get_ptr_opcode(lang::ASTNodeType node_type);
};

} // namespace ir_builder

#endif
