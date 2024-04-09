#include "compound_assign_ir_builder.hpp"

#include "ast/ast_child_indices.hpp"
#include "ast/ast_node.hpp"
#include "ir_builder/expr_ir_builder.hpp"
#include "utils/macros.hpp"

namespace ir_builder {

void CompoundAssignIRBuilder::build() {
    lang::ASTNode *location_node = node->get_child(lang::ASSIGN_LOCATION);
    lang::ASTNode *value_node = node->get_child(lang::ASSIGN_VALUE);

    ir::Value left_ptr = ExprIRBuilder(context, location_node).build_into_ptr();
    ir::VirtualRegister left_value_reg = context.append_load(left_ptr);
    ir::Value left_value = ir::Value::from_register(left_value_reg, left_ptr.get_type().deref());

    ir::Value right_value = ExprIRBuilder(context, value_node).build_into_value_if_possible();

    ir::VirtualRegister result_reg = context.get_current_func()->next_virtual_reg();
    ir::Value result_value = ir::Value::from_register(result_reg, left_ptr.get_type().deref());
    ir::Opcode opcode = get_opcode(node->get_type(), result_value.get_type());
    context.get_cur_block().append(ir::Instruction(opcode, result_reg, {left_value, right_value}));

    context.append_store(result_value, left_ptr);
}

ir::Opcode CompoundAssignIRBuilder::get_opcode(lang::ASTNodeType node_type, const ir::Type &type) {
    if (type.get_ptr_depth() != 0) {
        return get_ptr_opcode(node_type);
    } else if (type.is_floating_point()) {
        return get_fp_opcode(node_type);
    } else {
        return get_int_opcode(node_type);
    }
}

ir::Opcode CompoundAssignIRBuilder::get_int_opcode(lang::ASTNodeType node_type) {
    switch (node_type) {
        case lang::AST_ADD_ASSIGN: return ir::Opcode::ADD;
        case lang::AST_SUB_ASSIGN: return ir::Opcode::SUB;
        case lang::AST_MUL_ASSIGN: return ir::Opcode::MUL;
        case lang::AST_DIV_ASSIGN: return ir::Opcode::SDIV;
        case lang::AST_MOD_ASSIGN: return ir::Opcode::SREM;
        case lang::AST_BIT_AND_ASSIGN: return ir::Opcode::AND;
        case lang::AST_BIT_OR_ASSIGN: return ir::Opcode::OR;
        case lang::AST_BIT_XOR_ASSIGN: return ir::Opcode::XOR;
        case lang::AST_SHL_ASSIGN: return ir::Opcode::SHL;
        case lang::AST_SHR_ASSIGN: return ir::Opcode::SHR;
        default: ASSERT_UNREACHABLE;
    }
}

ir::Opcode CompoundAssignIRBuilder::get_fp_opcode(lang::ASTNodeType node_type) {
    switch (node_type) {
        case lang::AST_ADD_ASSIGN: return ir::Opcode::FADD;
        case lang::AST_SUB_ASSIGN: return ir::Opcode::FSUB;
        case lang::AST_MUL_ASSIGN: return ir::Opcode::FMUL;
        case lang::AST_DIV_ASSIGN: return ir::Opcode::FDIV;
        default: ASSERT_UNREACHABLE;
    }
}

ir::Opcode CompoundAssignIRBuilder::get_ptr_opcode(lang::ASTNodeType node_type) {
    switch (node_type) {
        case lang::AST_ADD_ASSIGN: return ir::Opcode::OFFSETPTR;
        default: ASSERT_UNREACHABLE;
    }
}

} // namespace ir_builder
