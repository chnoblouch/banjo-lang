#ifndef BLOCK_ANALYZER_H
#define BLOCK_ANALYZER_H

#include "ast/ast_node.hpp"
#include "sema/semantic_analyzer_context.hpp"

namespace lang {

class BlockAnalyzer {

private:
    ASTNode *node;
    SemanticAnalyzerContext &context;

public:
    BlockAnalyzer(ASTNode *node, SemanticAnalyzerContext &context);
    bool check();

private:
    bool analyze_var_decl(ASTNode *node);
    bool analyze_type_infer_var_decl(ASTNode *node);
    bool analyze_assign(ASTNode *node);
    bool analyze_compound_assign(ASTNode *node);
    bool analyze_if(ASTNode *node);
    bool analyze_switch(ASTNode *node);
    bool analyze_try(ASTNode *node);
    bool analyze_while(ASTNode *node);
    bool analyze_for(ASTNode *node);
    bool analyze_return(ASTNode *node);
    bool analyze_location(ASTNode *node);

    bool check_redefinition(SymbolTable *symbol_table, ASTNode *name_node);
};

} // namespace lang

#endif
