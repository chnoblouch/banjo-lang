#ifndef USE_COLLECTOR_H
#define USE_COLLECTOR_H

#include "ast/ast_node.hpp"
#include "sema/semantic_analyzer_context.hpp"

namespace banjo {

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
    void run(ASTNode *node);

private:
    void analyze_names(ASTNode *node, bool final_comp, SymbolTable *symbol_table);
    void analyze_ident_names(ASTNode *node, bool final_comp, SymbolTable *symbol_table);
    void analyze_dot_names(ASTNode *node, bool final_comp, SymbolTable *symbol_table);
    void analyze_tree_list(ASTNode *node, SymbolTable *symbol_table);
    void analyze_rebinding(ASTNode *node, bool final_comp, SymbolTable *symbol_table);
};

} // namespace lang

} // namespace banjo

#endif
