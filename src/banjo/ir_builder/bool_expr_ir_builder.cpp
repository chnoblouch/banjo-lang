#include "bool_expr_ir_builder.hpp"

#include "banjo/ast/ast_node.hpp"
#include "banjo/ir_builder/expr_ir_builder.hpp"

namespace banjo {

namespace ir_builder {

void BoolExprIRBuilder::build() {
    // TODO: unsigned comparisons

    switch (node->get_type()) {
        case lang::AST_TRUE: context.append_jmp(true_block); break;
        case lang::AST_FALSE: context.append_jmp(false_block); break;
        case lang::AST_OPERATOR_EQ: build_comparison(ir::Comparison::EQ, ir::Comparison::FEQ); break;
        case lang::AST_OPERATOR_NE: build_comparison(ir::Comparison::NE, ir::Comparison::FNE); break;
        case lang::AST_OPERATOR_GT: build_comparison(ir::Comparison::SGT, ir::Comparison::FGT); break;
        case lang::AST_OPERATOR_GE: build_comparison(ir::Comparison::SGE, ir::Comparison::FGE); break;
        case lang::AST_OPERATOR_LT: build_comparison(ir::Comparison::SLT, ir::Comparison::FLT); break;
        case lang::AST_OPERATOR_LE: build_comparison(ir::Comparison::SLE, ir::Comparison::FLE); break;
        case lang::AST_OPERATOR_AND: build_and(); break;
        case lang::AST_OPERATOR_OR: build_or(); break;
        case lang::AST_OPERATOR_NOT: build_not(); break;
        default: build_bool_eval(); break;
    }
}

void BoolExprIRBuilder::build_and() {
    int id = context.next_and_id();
    ir::BasicBlockIter rhs_block = context.create_block("and.right." + std::to_string(id));

    BoolExprIRBuilder(context, node->get_child(0), rhs_block, false_block).build();
    context.append_block(rhs_block);
    BoolExprIRBuilder(context, node->get_child(1), true_block, false_block).build();
}

void BoolExprIRBuilder::build_or() {
    int id = context.next_or_id();
    ir::BasicBlockIter rhs_block = context.create_block("or.right." + std::to_string(id));

    BoolExprIRBuilder(context, node->get_child(0), true_block, rhs_block).build();
    context.append_block(rhs_block);
    BoolExprIRBuilder(context, node->get_child(1), true_block, false_block).build();
}

void BoolExprIRBuilder::build_not() {
    BoolExprIRBuilder(context, node->get_child(), false_block, true_block).build();
}

void BoolExprIRBuilder::build_comparison(ir::Comparison int_comparison, ir::Comparison fp_comparison) {
    if (node->get_child(0)->as<lang::Expr>()->get_data_type()->get_kind() == lang::DataType::Kind::STRUCT) {
        ir::Value val = ExprIRBuilder(context, node).build_into_value().get_value();
        build_bool_eval(val);
        return;
    }

    ir::Value lhs_val = ExprIRBuilder(context, node->get_child(0)).build_into_value().get_value();
    ir::Value rhs_val = ExprIRBuilder(context, node->get_child(1)).build_into_value().get_value();
    bool is_fp = lhs_val.get_type().is_floating_point();

    ir::Opcode opcode = get_cmp_opcode(is_fp);
    ir::Operand cmp = ir::Operand::from_comparison(is_fp ? fp_comparison : int_comparison);
    ir::Operand true_target = ir::Operand::from_branch_target({.block = true_block, .args = {}});
    ir::Operand false_target = ir::Operand::from_branch_target({.block = false_block, .args = {}});
    context.get_cur_block().append(ir::Instruction(opcode, {lhs_val, cmp, rhs_val, true_target, false_target}));

    context.has_branch = true;
}

void BoolExprIRBuilder::build_bool_eval() {
    build_bool_eval(ExprIRBuilder(context, node).build_into_value().get_value());
}

void BoolExprIRBuilder::build_bool_eval(ir::Value value) {
    bool is_fp = value.get_type().is_floating_point();

    ir::Opcode opcode = get_cmp_opcode(is_fp);
    ir::Operand cmp = ir::Operand::from_comparison(is_fp ? ir::Comparison::FNE : ir::Comparison::NE);
    ir::Operand zero = ir::Operand::from_int_immediate(0);
    ir::Operand true_target = ir::Operand::from_branch_target({.block = true_block, .args = {}});
    ir::Operand false_target = ir::Operand::from_branch_target({.block = false_block, .args = {}});
    context.get_cur_block().append(ir::Instruction(opcode, {std::move(value), cmp, zero, true_target, false_target}));

    context.has_branch = true;
}

ir::Opcode BoolExprIRBuilder::get_cmp_opcode(bool is_fp) {
    return is_fp ? ir::Opcode::FCJMP : ir::Opcode::CJMP;
}

} // namespace ir_builder

} // namespace banjo
