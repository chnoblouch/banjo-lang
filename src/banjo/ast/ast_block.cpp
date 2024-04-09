#include "ast/ast_block.hpp"

namespace lang {

ASTBlock::ASTBlock() : ASTNode(AST_BLOCK), symbol_table(nullptr) {}

ASTBlock::ASTBlock(TextRange range, SymbolTable *parent_symbol_table)
  : ASTNode(AST_BLOCK, range),
    symbol_table(parent_symbol_table) {}

void ASTBlock::set_parent_symbol_table(SymbolTable *parent_symbol_table) {
    symbol_table.set_parent(parent_symbol_table);
}

ASTNode *ASTBlock::create_clone() {
    return new ASTBlock(range, symbol_table.get_parent());
}

ASTBlock *ASTBlock::clone_block(SymbolTable *parent_symbol_table) {
    ASTBlock *clone = new ASTBlock(range, parent_symbol_table);

    for (ASTNode *child : children) {
        clone->append_child(clone_block_child(child, clone->get_symbol_table()));
    }

    return clone;
}

ASTNode *ASTBlock::clone_block_child(ASTNode *child, SymbolTable *parent_symbol_table) {
    if (child->get_type() == AST_BLOCK) {
        return child->as<ASTBlock>()->clone_block(parent_symbol_table);
    }

    assert(child->get_type() != AST_MODULE);

    ASTNode *clone = child->create_clone();
    clone->set_value(child->get_value());
    clone->set_range(child->get_range());
    clone->set_attribute_list(child->get_attribute_list() ? new AttributeList(*child->get_attribute_list()) : nullptr);

    for (ASTNode *child : child->get_children()) {
        clone->append_child(clone_block_child(child, parent_symbol_table));
    }

    return clone;
}

} // namespace lang
