#ifndef DECL_TYPE_ANALYZER_H
#define DECL_TYPE_ANALYZER_H

#include "ast/ast_node.hpp"
#include "ast/decl.hpp"
#include "sema/semantic_analyzer.hpp"
#include "symbol/union.hpp"

#include <optional>
#include <utility>

namespace lang {

class DeclTypeAnalyzer {

private:
    SemanticAnalyzer &sema;
    SemanticAnalyzerContext &context;
    const ASTContext &ast_context;

public:
    DeclTypeAnalyzer(SemanticAnalyzer &sema);
    void run(Symbol *symbol);

private:
    bool analyze_func(ASTFunc *node);
    bool analyze_native_func(ASTFunc *node);
    bool analyze_const(ASTConst *node);
    bool analyze_var(ASTVar *node);
    bool analyze_native_var(ASTVar *node);
    bool analyze_struct(ASTStruct *node);
    bool analyze_enum(ASTEnum *node);
    bool analyze_union(ASTUnion *node);
    bool analyze_type_alias(ASTTypeAlias *node);
    bool analyze_generic_func(ASTGenericFunc *node);
    bool analyze_generic_struct(ASTGenericStruct *node);

private:
    std::pair<FunctionType, SemaResult> analyze_func_type(ASTNode *param_list, ASTNode *return_type_node);
    std::vector<std::string_view> collect_param_names(ASTNode *param_list);
    void analyze_attrs(ASTNode *node, Function *func);
    std::optional<std::vector<DataType *>> analyze_params(ASTNode *param_list);
    void analyze_attrs(ASTNode *node, GlobalVariable *global);
    void collect_generic_params(ASTNode *generic_param_list, std::vector<GenericParam> &generic_params);
};

} // namespace lang

#endif
