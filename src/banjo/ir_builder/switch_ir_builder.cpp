#include "switch_ir_builder.hpp"

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
#include "symbol/data_type.hpp"
#include "symbol/symbol_table.hpp"

#include <string>

namespace ir_builder {

void SwitchIRBuilder::build() {
    lang::ASTNode *value_node = node->get_child(lang::SWITCH_VALUE);
    lang::ASTNode *cases_node = node->get_child(lang::SWITCH_CASES);

    int id = context.next_switch_id();
    std::string id_str = std::to_string(id);

    StoredValue val = ExprIRBuilder(context, value_node).build_into_ptr();
    ir::VirtualRegister tag_ptr_reg = context.append_memberptr(val.value_type, val.get_ptr(), 0);
    ir::Value tag_ptr = ir::Value::from_register(tag_ptr_reg, ir::Primitive::ADDR);
    ir::Value tag = context.append_load(ir::Primitive::I32, tag_ptr);

    ir::VirtualRegister data_ptr_reg = context.append_memberptr(val.value_type, val.get_ptr(), 1);
    ir::Value data_ptr = ir::Value::from_register(data_ptr_reg, ir::Primitive::ADDR);

    ir::BasicBlockIter exit_iter = context.create_block("switch.exit." + id_str);

    for (unsigned i = 0; i < cases_node->get_children().size(); i++) {
        lang::ASTNode *case_node = cases_node->get_child(i);
        std::string case_id_str = std::to_string(i);

        ir::BasicBlockIter else_block;
        if (i == cases_node->get_children().size() - 1) {
            else_block = exit_iter;
        } else {
            else_block = context.create_block("switch.else." + id_str + "." + case_id_str);
        }

        if (case_node->get_type() == lang::AST_SWITCH_DEFAULT_CASE) {
            lang::ASTNode *block_node = case_node->get_child(0);

            BlockIRBuilder(context, block_node).build();
            context.append_jmp(exit_iter);
            context.append_block(else_block);

            continue;
        }

        lang::ASTNode *name_node = case_node->get_child(0);
        lang::ASTNode *type_node = case_node->get_child(1);
        lang::ASTNode *block_node = case_node->get_child(2);

        ir::BasicBlockIter then_block = context.create_block("switch.then." + id_str + "." + case_id_str);

        lang::UnionCase *lang_case = type_node->as<lang::Expr>()->get_data_type()->get_union_case();
        unsigned case_index = lang_case->get_union()->get_case_index(lang_case);
        ir::Operand case_tag = ir::Operand::from_int_immediate(case_index, ir::Primitive::I32);
        context.append_cjmp(tag, ir::Comparison::EQ, case_tag, then_block, else_block);

        context.append_block(then_block);

        BlockIRBuilder block_builder(context, block_node);
        block_builder.alloc_local_vars();

        lang::SymbolTable *symbol_table = block_node->as<lang::ASTBlock>()->get_symbol_table();
        lang::LocalVariable *lang_var = symbol_table->get_symbol(name_node->get_value())->get_local();
        StoredValue val = lang_var->as_ir_value(context);
        context.append_copy(val.get_ptr(), data_ptr, val.value_type);

        block_builder.build();
        context.append_jmp(exit_iter);
        context.append_block(else_block);
    }
}

} // namespace ir_builder
