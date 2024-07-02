#include "if_chain_ir_builder.hpp"

#include "banjo/ast/ast_child_indices.hpp"
#include "banjo/config/config.hpp"
#include "banjo/ir_builder/block_ir_builder.hpp"
#include "banjo/ir_builder/bool_expr_ir_builder.hpp"
#include "banjo/ir_builder/expr_ir_builder.hpp"
#include "banjo/ir_builder/ir_builder_utils.hpp"

namespace banjo {

namespace ir_builder {

void IfChainIRBuilder::build() {
    branch_id = context.next_if_chain_id();
    end_block = context.create_block("if.end." + std::to_string(branch_id));

    build_branches();
    context.append_block(end_block);
}

void IfChainIRBuilder::build_branches() {
    for (unsigned i = 0; i < node->get_children().size(); i++) {
        lang::ASTNode *branch = node->get_child(i);

        if (branch->get_type() == lang::AST_IF || branch->get_type() == lang::AST_ELSE_IF) {
            lang::ASTNode *condition_node = branch->get_child(lang::IF_CONDITION);
            lang::ASTNode *block_node = branch->get_child(lang::IF_BLOCK);

            ir::BasicBlockIter then_block = context.create_block(get_then_label(i));
            ir::BasicBlockIter next_block = create_next_block(i);

            BoolExprIRBuilder(context, condition_node, then_block, next_block).build();
            context.append_block(then_block);
            BlockIRBuilder(context, block_node).build();
            context.append_jmp(end_block);

            if (next_block != end_block) {
                context.append_block(next_block);
            }
        } else if (branch->get_type() == lang::AST_ELSE) {
            lang::ASTNode *block_node = branch->get_child();

            BlockIRBuilder(context, block_node).build();
            context.append_jmp(end_block);
        }
    }
}

ir::BasicBlockIter IfChainIRBuilder::create_next_block(unsigned cur_index) {
    if (cur_index == node->get_children().size() - 1) {
        return end_block;
    }

    unsigned next_index = cur_index + 1;
    lang::ASTNode *next_branch = node->get_child(next_index);

    if (next_branch->get_type() == lang::AST_ELSE_IF) {
        std::string label = get_condition_label(next_index);
        return context.create_block(label);
    } else if (next_branch->get_type() == lang::AST_ELSE) {
        std::string label = "if.else." + std::to_string(branch_id);
        else_block = context.create_block(label);
        return else_block;
    }

    return nullptr;
}

std::string IfChainIRBuilder::get_condition_label(unsigned index) {
    return "if.condition." + std::to_string(branch_id) + "." + std::to_string(index);
}

std::string IfChainIRBuilder::get_then_label(unsigned index) {
    return "if.then." + std::to_string(branch_id) + "." + std::to_string(index);
}

} // namespace ir_builder

} // namespace banjo
