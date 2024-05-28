#ifndef DECL_BODY_ANALYZER_H
#define DECL_BODY_ANALYZER_H

#include "ast/ast_node.hpp"
#include "ast/decl.hpp"
#include "sema/semantic_analyzer.hpp"
#include "symbol/symbol.hpp"

namespace lang {

class DeclBodyAnalyzer {

private:
    SemanticAnalyzer &sema;
    SemanticAnalyzerContext &context;

public:
    DeclBodyAnalyzer(SemanticAnalyzer &sema);
    void run(Symbol *symbol);

private:
    void analyze_func(ASTFunc *node);
    void analyze_const(ASTConst *node);
    void analyze_var(ASTVar *node);
    void analyze_struct(ASTStruct *node);
    void analyze_union(ASTUnion *node);
};

} // namespace lang

#endif
