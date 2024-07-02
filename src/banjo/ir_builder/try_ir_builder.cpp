#include "try_ir_builder.hpp"

#include "ast/ast_block.hpp"
#include "ast/ast_child_indices.hpp"
#include "ast/ast_node.hpp"
#include "ast/expr.hpp"
#include "ir/basic_block.hpp"
#include "ir/comparison.hpp"
#include "ir/operand.hpp"
#include "ir/virtual_register.hpp"
#include "ir_builder/block_ir_builder.hpp"
#include "ir_builder/expr_ir_builder.hpp"
#include "ir_builder/storage.hpp"
#include "symbol/symbol_table.hpp"
#include "utils/macros.hpp"

#include <string>

namespace banjo {

namespace ir_builder {

void TryIRBuilder::build() {
    int id = context.next_switch_id();
    id_str = std::to_string(id);

    success_block = context.create_block("try.success." + id_str);
    exit_block = context.create_block("try.exit." + id_str);

    for (unsigned i = 0; i < node->get_children().size(); i++) {
        build_case(i);
    }
}

void TryIRBuilder::build_case(unsigned index) {
    ir::BasicBlockIter next_block;
    unsigned next_index = index + 1;

    if (index == node->get_children().size() - 1) {
        next_block = exit_block;
    } else if (node->get_child(next_index)->get_type() == lang::AST_TRY_ERROR_CASE) {
        next_block = context.create_block("try.error." + id_str);
    } else if (node->get_child(next_index)->get_type() == lang::AST_TRY_ELSE_CASE) {
        next_block = context.create_block("try.else." + id_str);
    }

    lang::ASTNode *case_node = node->get_child(index);

    switch (case_node->get_type()) {
        case lang::AST_TRY_SUCCESS_CASE: build_success_case(case_node, next_block); break;
        case lang::AST_TRY_ERROR_CASE: build_error_case(case_node); break;
        case lang::AST_TRY_ELSE_CASE: build_else_case(case_node); break;
        default: ASSERT_UNREACHABLE;
    }

    context.append_block(next_block);
}

void TryIRBuilder::build_success_case(lang::ASTNode *node, ir::BasicBlockIter next_block) {
    lang::ASTNode *var_node = node->get_child(0);
    lang::ASTNode *value_node = node->get_child(1);
    lang::ASTBlock *block_node = node->get_child(2)->as<lang::ASTBlock>();

    value = ExprIRBuilder(context, value_node).build_into_ptr();
    ir::VirtualRegister member_ptr_reg = context.append_memberptr(value.value_type, value.get_ptr(), 0);
    ir::Value member_ptr = ir::Value::from_register(member_ptr_reg, ir::Primitive::ADDR);
    ir::Value success_flag = context.append_load(ir::Primitive::I8, member_ptr);

    ir::Operand false_operand = ir::Operand::from_int_immediate(0, ir::Primitive::I8);
    context.append_cjmp(success_flag, ir::Comparison::EQ, false_operand, next_block, success_block);

    context.append_block(success_block);

    ir::VirtualRegister value_ptr_reg = context.append_memberptr(value.value_type, value.get_ptr(), 1);
    ir::Value value_ptr = ir::Value::from_register(value_ptr_reg, ir::Primitive::ADDR);

    BlockIRBuilder block_builder(context, block_node);
    block_builder.alloc_local_vars();

    lang::SymbolTable *symbol_table = block_node->get_symbol_table();
    lang::LocalVariable *lang_var = symbol_table->get_symbol(var_node->get_value())->get_local();
    StoredValue val = lang_var->as_ir_value(context);
    StoredValue::create_reference(value_ptr, val.value_type).copy_to(val.get_ptr(), context);

    block_builder.build();

    context.append_jmp(exit_block);
}

void TryIRBuilder::build_error_case(lang::ASTNode *node) {
    lang::ASTNode *var_node = node->get_child(0);
    lang::ASTBlock *block_node = node->get_child(2)->as<lang::ASTBlock>();

    ir::VirtualRegister error_ptr_reg = context.append_memberptr(value.value_type, value.get_ptr(), 2);
    ir::Value error_ptr = ir::Value::from_register(error_ptr_reg, ir::Primitive::ADDR);

    BlockIRBuilder block_builder(context, block_node);
    block_builder.alloc_local_vars();

    lang::SymbolTable *symbol_table = block_node->get_symbol_table();
    lang::LocalVariable *lang_var = symbol_table->get_symbol(var_node->get_value())->get_local();
    StoredValue val = lang_var->as_ir_value(context);
    StoredValue::create_reference(error_ptr, val.value_type).copy_to(val.get_ptr(), context);

    block_builder.build();

    context.append_jmp(exit_block);
}

void TryIRBuilder::build_else_case(lang::ASTNode *node) {
    lang::ASTBlock *block_node = node->get_child(0)->as<lang::ASTBlock>();
    BlockIRBuilder(context, block_node).build();

    context.append_jmp(exit_block);
}

} // namespace ir_builder

} // namespace banjo
