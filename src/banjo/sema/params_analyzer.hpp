#ifndef PARAMS_ANALYZER_H
#define PARAMS_ANALYZER_H

#include "banjo/ast/ast_block.hpp"
#include "banjo/ast/ast_node.hpp"
#include "banjo/sema/semantic_analyzer_context.hpp"
#include "banjo/symbol/symbol_table.hpp"

namespace banjo {

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

} // namespace banjo

#endif
