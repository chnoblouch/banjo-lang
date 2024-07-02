#ifndef DECL_BODY_ANALYZER_H
#define DECL_BODY_ANALYZER_H

#include "banjo/ast/ast_node.hpp"
#include "banjo/ast/decl.hpp"
#include "banjo/sema/semantic_analyzer.hpp"
#include "banjo/symbol/symbol.hpp"

namespace banjo {

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

} // namespace banjo

#endif
