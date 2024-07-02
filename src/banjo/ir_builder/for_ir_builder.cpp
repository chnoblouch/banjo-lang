#include "for_ir_builder.hpp"

#include "banjo/ast/ast_block.hpp"
#include "banjo/ast/ast_child_indices.hpp"
#include "banjo/ast/ast_node.hpp"
#include "banjo/ast/expr.hpp"
#include "banjo/ir/virtual_register.hpp"
#include "banjo/ir_builder/block_ir_builder.hpp"
#include "banjo/ir_builder/expr_ir_builder.hpp"
#include "banjo/ir_builder/ir_builder_utils.hpp"
#include "banjo/ir_builder/storage.hpp"
#include "banjo/symbol/data_type.hpp"
#include "banjo/symbol/magic_functions.hpp"
#include "banjo/symbol/symbol_table.hpp"

namespace banjo {

namespace ir_builder {

void ForIRBuilder::build() {
    lang::ASTNode *var_node = node->get_child(lang::FOR_VAR);
    lang::ASTNode *expr_node = node->get_child(lang::FOR_EXPR);
    lang::ASTNode *block_node = node->get_child(lang::FOR_BLOCK);
    lang::SymbolTable *symbol_table = static_cast<lang::ASTBlock *>(block_node)->get_symbol_table();

    int id = context.next_for_id();
    entry = context.create_block("for.entry." + std::to_string(id));
    block = context.create_block("for.block." + std::to_string(id));
    exit = context.create_block("for.exit." + std::to_string(id));

    Scope &scope = context.push_scope();
    scope.loop_exit = exit;
    scope.loop_entry = entry;

    const std::string &var_name = var_node->get_value();
    var = symbol_table->get_symbol(var_name)->get_local();

    if (expr_node->get_type() == lang::AST_RANGE) {
        build_range_for();
    } else {
        build_iter_for();
    }

    context.pop_scope();
    context.has_branch = true;
}

void ForIRBuilder::build_range_for() {
    lang::ASTNode *expr_node = node->get_child(lang::FOR_EXPR);
    lang::ASTNode *block_node = node->get_child(lang::FOR_BLOCK);

    lang::ASTNode *start_node = expr_node->get_child(0);
    lang::ASTNode *end_node = expr_node->get_child(1);

    BlockIRBuilder block_builder(context, block_node);
    block_builder.alloc_local_vars();

    StoredValue counter_ptr = var->as_ir_value(context);
    ExprIRBuilder(context, start_node).build_and_store(counter_ptr.get_ptr().get_register());

    context.append_jmp(entry);
    context.append_block(entry);

    ir::Operand counter_cond = context.append_load(counter_ptr.value_type, counter_ptr.get_ptr());
    ir::Value end = ExprIRBuilder(context, end_node).build_into_value_if_possible().value_or_ptr;

    context.append_cjmp(counter_cond, ir::Comparison::NE, end, block, exit);
    context.append_block(block);
    block_builder.build_children();

    ir::Operand counter_inc = context.append_load(counter_ptr.value_type, counter_ptr.get_ptr());

    ir::VirtualRegister inc_result_reg = context.get_current_func()->next_virtual_reg();
    ir::Operand one = ir::Operand::from_int_immediate(1, counter_inc.get_type());
    context.get_cur_block().append(ir::Instruction(ir::Opcode::ADD, inc_result_reg, {counter_inc, one}));
    ir::Operand inc_result = ir::Operand::from_register(inc_result_reg, counter_inc.get_type());

    context.get_cur_block().append(ir::Instruction(ir::Opcode::STORE, {inc_result, counter_ptr.get_ptr()}));

    context.append_jmp(entry);
    context.append_block(exit);
}

void ForIRBuilder::build_iter_for() {
    lang::ASTNode *iter_type_node = node->get_child(lang::FOR_ITER_TYPE);
    lang::ASTNode *expr_node = node->get_child(lang::FOR_EXPR);
    lang::ASTNode *block_node = node->get_child(lang::FOR_BLOCK);

    bool is_by_ref = iter_type_node->get_value() == "*";

    BlockIRBuilder block_builder(context, block_node);
    block_builder.alloc_local_vars();

    ir::Value iterable = ExprIRBuilder(context, expr_node).build_into_ptr().get_ptr();
    lang::Structure *lang_struct = expr_node->as<lang::Expr>()->get_data_type()->get_structure();
    lang::Function *lang_iter_func = lang_struct->get_method_table().get_function(lang::MagicFunctions::ITER);
    IRBuilderUtils::FuncCall iter_call{lang_iter_func, {iterable}};
    ir::Value iter_ptr = IRBuilderUtils::build_call(iter_call, context).get_ptr();

    context.append_jmp(entry);
    context.append_block(entry);

    lang::DataType *lang_iter_type = lang_iter_func->get_type().return_type;
    lang::Structure *lang_iter_struct = lang_iter_type->get_structure();
    lang::Function *lang_next_func = lang_iter_struct->get_method_table().get_function(lang::MagicFunctions::NEXT);
    IRBuilderUtils::FuncCall next_call{lang_next_func, {iter_ptr}};
    ir::Value next_ptr = IRBuilderUtils::build_call(next_call, context).value_or_ptr;
    ir::Operand null = ir::Operand::from_int_immediate(0, ir::Primitive::ADDR);

    context.append_cjmp(next_ptr, ir::Comparison::NE, null, block, exit);
    context.append_block(block);

    StoredValue var_value = var->as_ir_value(context);
    if (is_by_ref) {
        context.append_store(next_ptr, var_value.get_ptr());
    } else {
        IRBuilderUtils::copy_val(context, next_ptr, var_value.get_ptr(), var_value.value_type);
    }

    block_builder.build_children();

    context.append_jmp(entry);
    context.append_block(exit);
}

} // namespace ir_builder

} // namespace banjo
