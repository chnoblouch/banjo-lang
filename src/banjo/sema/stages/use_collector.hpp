#ifndef USE_COLLECTOR_H
#define USE_COLLECTOR_H

#include "ast/ast_node.hpp"
#include "sema/semantic_analyzer_context.hpp"
#include "symbol/symbol_ref.hpp"

namespace lang {

class UseCollector {

private:
    struct PathContext {
        bool first;
        bool final_comp;
    };

private:
    SemanticAnalyzerContext &context;
    ASTNode *use_node;

public:
    UseCollector(SemanticAnalyzerContext &context);
    void analyze_names(ASTNode *node);
    void resolve_names(ASTNode *node);

private:
    void analyze_names(ASTNode *node, PathContext ctx, SymbolTable *symbol_table);
    void analyze_ident_names(ASTNode *node, PathContext ctx, SymbolTable *symbol_table);
    void analyze_dot_names(ASTNode *node, PathContext ctx, SymbolTable *symbol_table);
    void analyze_tree_list(ASTNode *node, PathContext ctx, SymbolTable *symbol_table);
    void analyze_rebinding(ASTNode *node, PathContext ctx, SymbolTable *symbol_table);

    void resolve_names(ASTNode *node, PathContext ctx, SymbolTable *&symbol_table);
    void resolve_ident_names(ASTNode *node, PathContext ctx, SymbolTable *&symbol_table);
    void resolve_dot_names(ASTNode *node, PathContext ctx, SymbolTable *&symbol_table);
    void resolve_tree_list(ASTNode *node, PathContext ctx, SymbolTable *&symbol_table);
    void resolve_rebinding(ASTNode *node, PathContext ctx, SymbolTable *&symbol_table);

    void add_dependency(ASTModule *mod);
};

} // namespace lang

#endif
