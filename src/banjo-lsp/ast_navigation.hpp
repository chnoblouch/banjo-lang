#ifndef BANJO_LSP_AST_NAVIGATION_H
#define BANJO_LSP_AST_NAVIGATION_H

#include "banjo/ast/ast_node.hpp"

namespace banjo {

namespace lsp {

struct LSPTextPosition {
    int line;
    int column;
};

struct LSPTextRange {
    LSPTextPosition start;
    LSPTextPosition end;
};

namespace ASTNavigation {

lang::TextPosition pos_from_lsp(const std::string &source, int line, int column);
LSPTextPosition pos_to_lsp(const std::string &source, lang::TextPosition position);

lang::ASTNode *get_node_at(lang::ASTNode *node, lang::TextPosition position);

} // namespace ASTNavigation

} // namespace lsp

} // namespace banjo

#endif
