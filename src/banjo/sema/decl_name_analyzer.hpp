#ifndef DECL_NAME_ANALYZER_H
#define DECL_NAME_ANALYZER_H

#include "ast/ast_node.hpp"
#include "ast/decl.hpp"
#include "ast/module_list.hpp"
#include "sema/semantic_analyzer.hpp"

namespace lang {

class DeclNameAnalyzer {

private:
    SemanticAnalyzer &sema;
    bool modify_symbol_table;

public:
    DeclNameAnalyzer(SemanticAnalyzer &sema, bool modify_symbol_table = true);
    void run(ModuleList &module_list);
    void run(ASTBlock *block);
    void analyze_symbol(ASTNode *node);

private:
    void analyze_func(ASTFunc *node);
    void analyze_const(ASTConst *node);
    void analyze_var(ASTVar *node);
    void analyze_implicit_type_var(ASTVar *node);
    void analyze_struct(ASTStruct *node);
    void analyze_enum(ASTEnum *node);
    void analyze_union(ASTUnion *node);
    void analyze_proto(ASTProto *node);
    void analyze_type_alias(ASTTypeAlias *node);
    void analyze_generic_func(ASTGenericFunc *node);
    void analyze_generic_struct(ASTGenericStruct *node);
};

} // namespace lang

#endif
