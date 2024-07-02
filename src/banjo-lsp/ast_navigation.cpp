#include "ast_navigation.hpp"

namespace banjo {

namespace lsp {

lang::TextPosition ASTNavigation::pos_from_lsp(const std::string &source, int line, int column) {
    lang::TextPosition offset = 0;

    for (int source_line = 0; source_line < line; source_line++) {
        while (source[offset] != '\n') {
            offset++;
        }
        offset++;
    }

    for (int source_column = 0; source_column < column; source_column++) {
        offset++;
    }

    return offset;
}

LSPTextPosition ASTNavigation::pos_to_lsp(const std::string &source, lang::TextPosition position) {
    LSPTextPosition lsp_position{.line = 0, .column = 0};

    for (lang::TextPosition offset = 0; offset < position; offset++) {
        if (source[offset] == '\n') {
            lsp_position.line++;
            lsp_position.column = 0;
        } else {
            lsp_position.column++;
        }
    }

    return lsp_position;
}

lang::ASTNode *ASTNavigation::get_node_at(lang::ASTNode *node, lang::TextPosition position) {
    for (lang::ASTNode *child : node->get_children()) {
        if (position >= child->get_range().start && position <= child->get_range().end) {
            return get_node_at(child, position);
        }
    }

    return node;
}

} // namespace lsp

} // namespace banjo
