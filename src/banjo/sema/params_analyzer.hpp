#ifndef PARAMS_ANALYZER_H
#define PARAMS_ANALYZER_H

#include "ast/ast_block.hpp"
#include "ast/ast_node.hpp"
#include "sema/semantic_analyzer_context.hpp"
#include "symbol/symbol_table.hpp"

namespace lang {

class ParamsAnalyzer {

private:
    SemanticAnalyzerContext &context;

public:
    ParamsAnalyzer(SemanticAnalyzerContext &context);
    bool analyze(ASTNode *node, ASTBlock *block);
    bool check_redefinition(SymbolTable *symbol_table, ASTNode *name_node);
};

} // namespace lang

#endif
