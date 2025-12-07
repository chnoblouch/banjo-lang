#ifndef BANJO_PARSER_NODE_BUILDER_H
#define BANJO_PARSER_NODE_BUILDER_H

#include "banjo/ast/ast_module.hpp"
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
    TokenStream &stream;
    ASTModule *mod;
    ASTNode *node;
    std::vector<unsigned> tokens;

public:
    NodeBuilder(TokenStream &stream, ASTModule *mod, ASTNode *node) : stream(stream), mod(mod), node(node) {
        node->range.start = stream.get()->position;
    }

    void consume() {
        stream.consume();
        tokens.push_back(stream.get_position() - 1);
    }

    void append_child(ASTNode *child) { node->append_child(child); }
    void set_start_position(TextPosition start_position) { node->range.start = start_position; }

    ASTNode *build(ASTNodeType type) {
        Token *previous = stream.previous();
        node->range.end = previous ? previous->end() : node->range.start;
        node->type = type;
        node->tokens = mod->create_token_indices(tokens);
        return node;
    }

    ASTNode *build_with_inferred_range(ASTNodeType type) {
        ASTNode *node = build(type);
        node->set_range_from_children();
        return node;
    }

    ParseResult build_error(ASTNodeType type = AST_ERROR) {
        if (node->last_child) {
            node->range.end = node->last_child->range.end;
        }

        node->type = type;
        return ParseResult{node, false};
    }
};

} // namespace lang

} // namespace banjo

#endif
