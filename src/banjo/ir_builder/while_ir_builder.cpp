#include "while_ir_builder.hpp"

#include "ast/ast_child_indices.hpp"
#include "ir_builder/block_ir_builder.hpp"
#include "ir_builder/bool_expr_ir_builder.hpp"

namespace ir_builder {

void WhileIRBuilder::build() {
    lang::ASTNode *condition_node = node->get_child(lang::WHILE_CONDITION);
    lang::ASTNode *block_node = node->get_child(lang::WHILE_BLOCK);

    int id = context.next_while_id();
    ir::BasicBlockIter entry_block = context.create_block("while.entry." + std::to_string(id));
    ir::BasicBlockIter then_block = context.create_block("while.block." + std::to_string(id));
    ir::BasicBlockIter exit_block = context.create_block("while.exit." + std::to_string(id));

    Scope &scope = context.push_scope();
    scope.loop_exit = exit_block;
    scope.loop_entry = entry_block;

    context.append_jmp(entry_block);
    context.append_block(entry_block);
    BoolExprIRBuilder(context, condition_node, then_block, exit_block).build();

    context.append_block(then_block);
    BlockIRBuilder(context, block_node).build();

    context.append_jmp(entry_block);
    context.append_block(exit_block);

    context.pop_scope();
}

} // namespace ir_builder
