#include "token_list.hpp"

#include "banjo/ast/ast_node.hpp"

namespace banjo::lang {

std::span<Token> TokenList::get_attached_tokens(unsigned token_index) {
    TokenList::Span span = attachments[token_index];
    return std::span<Token>{&attached_tokens[span.first], span.count};
}

unsigned TokenList::last_token_index(ASTNode *node) {
    if (!node->has_children()) {
        return node->tokens[0];
    } else if (node->tokens.empty()) {
        return last_token_index(node->last_child);
    }

    unsigned last_parent_token = node->tokens.back();
    unsigned last_child_token = last_token_index(node->last_child);

    if (tokens[last_parent_token].position > tokens[last_child_token].position) {
        return last_parent_token;
    } else {
        return last_child_token;
    }
}

} // namespace banjo::lang
