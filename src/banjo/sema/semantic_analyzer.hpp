#ifndef SEMANTIC_ANALYZER_H
#define SEMANTIC_ANALYZER_H

#include "ast/ast_node.hpp"
#include "sema/semantic_analyzer_context.hpp"
#include "source/module_manager.hpp"
#include "symbol/data_type_manager.hpp"
#include "symbol/symbol.hpp"

#include <functional>
#include <optional>
#include <string>

namespace banjo {

namespace lang {

class SemanticAnalyzer {

private:
    SemanticAnalyzerContext context;

public:
    SemanticAnalyzer(ModuleManager &module_manager, DataTypeManager &type_manager, target::Target *target);
    SemanticAnalyzerContext &get_context() { return context; }

    void run_name_stage(ASTBlock *block);
    void run_meta_counting_stage(ASTBlock *block);
    void run_use_resolution_stage(ASTBlock *block);
    void run_meta_expansion_stage(ASTBlock *block);
    void run_type_stage(ASTNode *node);
    void run_body_stage(ASTNode *node);

    void run_name_stage(ASTModule *module_);

    void run_name_stage(ASTNode *node, bool modify_symbol_table = true);
    void run_use_resolution_stage(ASTNode *node);

    std::optional<SymbolRef> resolve_symbol(SymbolTable *symbol_table, const std::string &name);

    SemanticAnalysis analyze();
    SemanticAnalysis analyze_modules(const std::vector<ASTModule *> &module_nodes);
    SemanticAnalysis analyze_module(ASTModule *module_);

    void enable_dependency_tracking();
    void enable_symbol_use_tracking();

private:
    void for_each_module(const std::function<void(ASTModule *module_)> &function);

    void for_each_module(
        const std::vector<ASTModule *> &modules,
        const std::function<void(ASTModule *module_)> &function
    );
};

} // namespace lang

} // namespace banjo

#endif
