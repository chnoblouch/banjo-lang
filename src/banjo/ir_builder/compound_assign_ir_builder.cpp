#include "compound_assign_ir_builder.hpp"

#include "ast/ast_child_indices.hpp"
#include "ast/ast_node.hpp"
#include "ir/virtual_register.hpp"
#include "ir_builder/expr_ir_builder.hpp"
#include "ir_builder/ir_builder_utils.hpp"
#include "utils/macros.hpp"

namespace banjo {

namespace ir_builder {

void CompoundAssignIRBuilder::build() {
    lang::Expr *location_node = node->get_child(lang::ASSIGN_LOCATION)->as<lang::Expr>();
    lang::Expr *value_node = node->get_child(lang::ASSIGN_VALUE)->as<lang::Expr>();

    StoredValue left_ptr = ExprIRBuilder(context, location_node).build_into_ptr();
    ir::Value left_value = context.append_load(left_ptr.value_type, left_ptr.get_ptr());

    ir::Value right_value = ExprIRBuilder(context, value_node).build_into_value_if_possible().value_or_ptr;

    if (location_node->get_data_type()->get_kind() != lang::DataType::Kind::POINTER) {
        ir::VirtualRegister result_reg = context.get_current_func()->next_virtual_reg();
        ir::Value result_value = ir::Value::from_register(result_reg, left_ptr.value_type);
        ir::Opcode opcode = get_opcode(node->get_type(), result_value.get_type());
        context.get_cur_block().append(ir::Instruction(opcode, result_reg, {left_value, right_value}));
        context.append_store(result_value, left_ptr.get_ptr());
    } else {
        ir::Type base_type = context.build_type(location_node->get_data_type()->get_base_data_type());
        ir::VirtualRegister result_reg = context.append_offsetptr(left_value, right_value, base_type);
        ir::Value result_value = ir::Value::from_register(result_reg, left_ptr.value_type);
        context.append_store(result_value, left_ptr.get_ptr());
    }
}

ir::Opcode CompoundAssignIRBuilder::get_opcode(lang::ASTNodeType node_type, const ir::Type &type) {
    return type.is_floating_point() ? get_fp_opcode(node_type) : get_int_opcode(node_type);
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

} // namespace ir_builder

} // namespace banjo
