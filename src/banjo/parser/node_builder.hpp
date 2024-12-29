#ifndef NODE_BUILDER_H
#define NODE_BUILDER_H

#include "banjo/ast/ast_node.hpp"
#include "banjo/parser/token_stream.hpp"

#include <memory>
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
    TokenStream &stream;
    ASTNode *node;

public:
    NodeBuilder(TokenStream &stream, ASTNode *node) : stream(stream), node(node) {
        node->range.start = stream.get()->get_position();
    }

    void append_child(ASTNode *child) { node->append_child(child); }

    void set_attribute_list(std::unique_ptr<AttributeList> attribute_list) {
        node->attribute_list = std::move(attribute_list);
    }

    void set_start_position(TextPosition start_position) { node->range.start = start_position; }

    ASTNode *build(ASTNodeType type) {
        Token *previous = stream.previous();
        node->range.end = previous ? previous->get_end() : node->range.start;

        node->type = type;
        return node;
    }

    ASTNode *build_with_inferred_range(ASTNodeType type) {
        ASTNode *node = build(type);
        node->set_range_from_children();
        return node;
    }

    ParseResult build_error() {
        if (!node->children.empty()) {
            node->range.end = node->children.back()->get_range().end;
        }

        node->type = AST_ERROR;
        return node;
    }
};

} // namespace lang

} // namespace banjo

#endif
