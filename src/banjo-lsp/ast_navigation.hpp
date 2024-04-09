#ifndef LSP_AST_NAVIGATION_H
#define LSP_AST_NAVIGATION_H

#include "ast/ast_node.hpp"
#include "source_manager.hpp"

namespace lsp {

namespace ASTNavigation {

lang::TextPosition pos_from_lsp(const std::string &source, int line, int column);
LSPTextPosition pos_to_lsp(const std::string &source, lang::TextPosition position);

lang::ASTNode *get_node_at(lang::ASTNode *node, lang::TextPosition position);

} // namespace ASTNavigation

} // namespace lsp

#endif
