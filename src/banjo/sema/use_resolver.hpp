#ifndef USE_RESOLVER_H
#define USE_RESOLVER_H

#include "ast/ast_node.hpp"
#include "sema/semantic_analyzer_context.hpp"

namespace lang {

class UseResolver {

private:
    struct PathContext {
        bool first;
        bool final_comp;
    };

    SemanticAnalyzerContext &context;
    ASTNode *use_node;

public:
    UseResolver(SemanticAnalyzerContext &context);
    void run(ASTNode *node);

    void resolve_names(ASTNode *node, PathContext ctx, SymbolTable *&symbol_table);
    void resolve_ident_names(ASTNode *node, PathContext ctx, SymbolTable *&symbol_table);
    void resolve_dot_names(ASTNode *node, PathContext ctx, SymbolTable *&symbol_table);
    void resolve_tree_list(ASTNode *node, SymbolTable *&symbol_table);
    void resolve_rebinding(ASTNode *node, PathContext ctx, SymbolTable *&symbol_table);
};

} // namespace lang

#endif
