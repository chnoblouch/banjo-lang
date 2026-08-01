#ifndef BANJO_LSP_AST_NAVIGATION_H
#define BANJO_LSP_AST_NAVIGATION_H

#include "banjo/ast/ast_node.hpp"

#include <string_view>

namespace banjo::lsp {

struct LSPTextPosition {
    int line;
    int column;
};

struct LSPTextRange {
    LSPTextPosition start;
    LSPTextPosition end;
};

namespace ASTNavigation {

TextPosition pos_from_lsp(std::string_view source, int line, int column);
LSPTextPosition pos_to_lsp(std::string_view source, TextPosition position);

ASTNode *get_node_at(ASTNode *node, TextPosition position);

} // namespace ASTNavigation

} // namespace banjo::lsp

#endif
