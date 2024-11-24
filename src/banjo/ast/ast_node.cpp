#include "ast_node.hpp"

#include "banjo/source/text_range.hpp"

#include <algorithm>
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

ASTNode::ASTNode(ASTNodeType type, Token *token) : type(type), value(token->get_value()), range(token->get_range()) {}

ASTNode::~ASTNode() {
    for (ASTNode *child : children) {
        delete child;
    }

    delete attribute_list;
}

ASTNode *ASTNode::get_child_of_type(ASTNodeType type) {
    for (ASTNode *child : children) {
        if (child->get_type() == type) {
            return child;
        }
    }

    return nullptr;
}

bool ASTNode::has_child(unsigned index) {
    return children.size() >= index + 1;
}

void ASTNode::append_child(ASTNode *child) {
    children.push_back(child);
    child->parent = this;
}

void ASTNode::insert_child(int index, ASTNode *child) {
    children.insert(children.begin() + index, child);
    child->parent = this;
}

void ASTNode::replace_child(ASTNode *old_child, ASTNode *new_child, bool keep_data) {
    std::replace(children.begin(), children.end(), old_child, new_child);
    new_child->parent = this;
    delete old_child;
}

void ASTNode::remove_child(int index) {
    delete children[index];
    children.erase(children.begin() + index);
}

ASTNode *ASTNode::detach_child(int index) {
    ASTNode *child = children[index];
    children.erase(children.begin() + index);
    return child;
}

int ASTNode::index_of_child(ASTNode *child) {
    for (unsigned i = 0; i < children.size(); i++) {
        if (children[i] == child) {
            return i;
        }
    }

    return -1;
}

bool ASTNode::is_ancestor_of(ASTNode *other) {
    ASTNode *current = other->get_parent();

    while (current) {
        if (current == this) {
            return true;
        } else {
            current = current->get_parent();
        }
    }

    return false;
}

void ASTNode::set_range_from_children() {
    if (!children.empty()) {
        range = {children.front()->get_range().start, children.back()->get_range().end};
    }
}

} // namespace lang

} // namespace banjo
