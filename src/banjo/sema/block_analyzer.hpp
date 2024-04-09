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
    bool check_var_decl(ASTNode *node);
    bool check_type_infer_var_decl(ASTNode *node);
    bool check_assign(ASTNode *node);
    bool check_compound_assign(ASTNode *node);
    bool check_if(ASTNode *node);
    bool check_switch(ASTNode *node);
    bool check_while(ASTNode *node);
    bool check_for(ASTNode *node);
    bool check_return(ASTNode *node);
    bool check_location(ASTNode *node);

    bool check_redefinition(SymbolTable *symbol_table, ASTNode *name_node);
};

} // namespace lang

#endif
