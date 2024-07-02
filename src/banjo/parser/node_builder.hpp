#ifndef NODE_BUILDER_H
#define NODE_BUILDER_H

#include "banjo/ast/ast_node.hpp"
#include "banjo/parser/token_stream.hpp"

#include <utility>

namespace banjo {

namespace lang {

struct ParseResult {
    ASTNode *node;
    bool is_valid;
    ParseResult(ASTNode *node = nullptr, bool is_valid = true) : node(node), is_valid(is_valid) {}
};

class NodeBuilder {

private:
    TextPosition start_position;
    std::vector<ASTNode *> children;
    AttributeList *attribute_list = nullptr;
    TokenStream &stream;

public:
    NodeBuilder(TokenStream &stream) : start_position(stream.get()->get_position()), stream(stream) {}

    void append_child(ASTNode *child) { children.push_back(child); }
    void set_attribute_list(AttributeList *attribute_list) { this->attribute_list = attribute_list; }
    void set_start_position(TextPosition start_position) { this->start_position = start_position; }

    template <typename T>
    T *build(T *node) {
        Token *previous = stream.previous();
        TextPosition end_position = previous ? previous->get_end() : start_position;
        TextRange range(start_position, end_position);

        for (ASTNode *child : children) {
            node->append_child(child);
        }

        node->set_range(range);
        node->set_attribute_list(attribute_list);
        return node;
    }

    template <typename T>
    T *build() {
        return build(new T);
    }

    ASTNode *build(ASTNodeType type) {
        ASTNode *node = build<ASTNode>();
        node->set_type(type);
        return node;
    }

    ASTNode *build_with_inferred_range(ASTNodeType type) {
        ASTNode *node = build(type);
        node->set_range_from_children();
        return node;
    }

    template <typename T>
    T *build_with_inferred_range(T *node) {
        build(node);
        node->set_range_from_children();
        return node;
    }

    ParseResult build_error() {
        ASTNode *node = new ASTNode(AST_ERROR);
        node->children = std::move(children);

        for (ASTNode *child : node->children) {
            child->parent = node;
        }

        if (!node->children.empty()) {
            node->range = {start_position, node->children.back()->get_range().end};
        }

        node->attribute_list = attribute_list;
        return node;
    }
};

} // namespace lang

} // namespace banjo

#endif
