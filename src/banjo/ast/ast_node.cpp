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
    range = token->get_range();
    value = token->move_value();
}

bool ASTNode::has_child(unsigned index) {
    return children.size() >= index + 1;
}

void ASTNode::append_child(ASTNode *child) {
    children.push_back(child);
    child->parent = this;
}

void ASTNode::set_range_from_children() {
    if (!children.empty()) {
        range = {children.front()->get_range().start, children.back()->get_range().end};
    }
}

} // namespace lang

} // namespace banjo
