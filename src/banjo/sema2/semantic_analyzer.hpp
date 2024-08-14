#ifndef SEMA2_SEMANTIC_ANALYZER_H
#define SEMA2_SEMANTIC_ANALYZER_H

#include "banjo/reports/report_manager.hpp"
#include "banjo/sema2/report_generator.hpp"
#include "banjo/sir/sir.hpp"
#include "banjo/target/target.hpp"

#include <functional>
#include <set>
#include <stack>
#include <string_view>
#include <unordered_map>
#include <utility>

namespace banjo {

namespace lang {

namespace sema {

struct Scope {
    sir::FuncDef *func_def = nullptr;
    sir::Block *block = nullptr;
    sir::DeclBlock *decl_block = nullptr;
    sir::StructDef *struct_def = nullptr;
    sir::EnumDef *enum_def = nullptr;
    std::unordered_map<std::string_view, sir::Expr> generic_args;
};

enum class Result {
    SUCCESS,
    ERROR,
    AWAITING_DEPENDENCY,
};

struct PostponedAnalysis {
    std::function<void()> callback;
    Scope context;
};

class SemanticAnalyzer {

    friend class SymbolCollector;
    friend class UseResolver;
    friend class DeclInterfaceAnalyzer;
    friend class DeclValueAnalyzer;
    friend class DeclBodyAnalyzer;
    friend class ExprAnalyzer;
    friend class StmtAnalyzer;
    friend class ConstEvaluator;
    friend class GenericsSpecializer;
    friend class MetaExpansion;
    friend class DeclVisitor;
    friend class Preamble;
    friend class ReportGenerator;
    friend class ReportBuilder;

private:
    sir::Unit &sir_unit;
    target::Target *target;
    ReportManager &report_manager;
    ReportGenerator report_generator;

    sir::Module *cur_sir_mod;
    std::stack<Scope> scopes;
    std::vector<PostponedAnalysis> postponed_analyses;
    std::set<const sir::Decl *> blocked_decls;

public:
    SemanticAnalyzer(sir::Unit &sir_unit, target::Target *target, ReportManager &report_manager);
    void analyze();

private:
    Scope &get_scope() { return scopes.top(); }

    Scope &push_scope() {
        scopes.push(scopes.top());
        return get_scope();
    }

    void pop_scope() { scopes.pop(); }

    void enter_mod(sir::Module *mod);
    void exit_mod() { scopes.pop(); }
    void enter_struct_def(sir::StructDef *struct_def);
    void exit_struct_def() { scopes.pop(); }
    void enter_enum_def(sir::EnumDef *enum_def);
    void exit_enum_def() { scopes.pop(); }

    bool is_in_stmt_block() { return get_scope().block; }
    sir::SymbolTable &get_symbol_table();
    
    sir::Symbol find_std_symbol(const ModulePath &mod_path, const std::string &name);
    sir::Symbol find_std_string();

    void check_for_completeness(sir::DeclBlock &block);
    void postpone_analysis(std::function<void()> function);
    void run_postponed_analyses();

    template <typename T>
    T *create_expr(T value) {
        return cur_sir_mod->create_expr(value);
    }

    template <typename T>
    T *create_stmt(T value) {
        return cur_sir_mod->create_stmt(value);
    }

    template <typename T>
    T *create_decl(T value) {
        return cur_sir_mod->create_decl(value);
    }

    template <typename T>
    T *create_use_item(T value) {
        return cur_sir_mod->create_use_item(value);
    }

    sir::SymbolTable *create_symbol_table(sir::SymbolTable value) {
        return cur_sir_mod->create_symbol_table(std::move(value));
    }
};

} // namespace sema

} // namespace lang

} // namespace banjo

#endif
