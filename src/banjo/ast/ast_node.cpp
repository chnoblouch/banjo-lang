#include "ast_node.hpp"

#include "banjo/source/text_range.hpp"

#include <utility>

namespace banjo {

namespace lang {

ASTNode::ASTNode() : type(AST_INVALID), value(""), range(0, 0) {}

ASTNode::ASTNode(ASTNodeType type) : type(type), value(""), range(0, 0) {}

ASTNode::ASTNode(ASTNodeType type, std::string value, TextRange range)
  : type(type),
    value(std::move(value)),
    range(range) {}

ASTNode::ASTNode(ASTNodeType type, TextRange range) : type(type), range(range) {}

ASTNode::ASTNode(ASTNodeType type, Token *token) : type(type), value(""), range{0, 0} {
    range = token->range();
    value = std::move(token->value);
}

unsigned ASTNode::num_children() {
    unsigned num_children = 0;

    for (ASTNode *child = first_child; child; child = child->next_sibling) {
        num_children += 1;
    }

    return num_children;
}

void ASTNode::append_child(ASTNode *child) {
    if (!first_child) {
        first_child = child;
    } else {
        last_child->next_sibling = child;
    }

    last_child = child;
}

void ASTNode::set_range_from_children() {
    if (!has_children()) {
        range = {0, 0};
        return;
    }

    range.start = first_child->range.start;

    for (ASTNode *child = first_child; child; child = child->next_sibling) {
        range.end = child->range.end;
    }
}

} // namespace lang

} // namespace banjo
