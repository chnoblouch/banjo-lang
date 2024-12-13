#ifndef sema_SEMANTIC_ANALYZER_H
#define sema_SEMANTIC_ANALYZER_H

#include "banjo/reports/report_manager.hpp"
#include "banjo/sema/completion_context.hpp"
#include "banjo/sema/extra_analysis.hpp"
#include "banjo/sema/report_generator.hpp"
#include "banjo/sir/sir.hpp"
#include "banjo/target/target.hpp"

#include <set>
#include <stack>
#include <string_view>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

namespace banjo {

namespace lang {

namespace sema {

enum class Mode {
    COMPILATION,
    INDEXING,
    COMPLETION,
};

struct ClosureContext {
    std::vector<sir::Symbol> captured_vars;
    sir::TupleExpr *data_type;
    sir::Block *parent_block;
};

struct Scope {
    sir::Symbol decl = nullptr;
    sir::Block *block = nullptr;
    std::unordered_map<std::string_view, sir::Expr> generic_args;
    ClosureContext *closure_ctx = nullptr;
};

enum class Result {
    SUCCESS,
    ERROR,
};

class SemanticAnalyzer {

    // This is getting slightly out of hand...
    friend class SymbolCollector;
    friend class UseResolver;
    friend class TypeAliasResolver;
    friend class DeclInterfaceAnalyzer;
    friend class DeclValueAnalyzer;
    friend class DeclBodyAnalyzer;
    friend class ResourceAnalyzer;
    friend class ExprAnalyzer;
    friend class ExprFinalizer;
    friend class StmtAnalyzer;
    friend class ConstEvaluator;
    friend class GenericsSpecializer;
    friend class GenericArgInference;
    friend class MetaExpansion;
    friend class MetaExprEvaluator;
    friend class DeclVisitor;
    friend class ReportGenerator;
    friend class ReportBuilder;

private:
    sir::Unit &sir_unit;
    target::Target *target;
    ReportManager &report_manager;
    ReportGenerator report_generator;
    Mode mode;

    ExtraAnalysis extra_analysis;
    CompletionContext completion_context;

    sir::Module *cur_sir_mod;
    std::stack<Scope> scopes;
    std::unordered_map<std::string_view, sir::Symbol> preamble_symbols;
    std::set<const sir::Decl *> blocked_decls;
    std::vector<std::tuple<sir::Decl, Scope>> decls_awaiting_body_analysis;
    std::unordered_map<sir::DeclBlock *, Scope> incomplete_decl_blocks;
    bool in_meta_expansion = false;
    unsigned loop_depth = 0;

public:
    SemanticAnalyzer(
        sir::Unit &sir_unit,
        target::Target *target,
        ReportManager &report_manager,
        Mode mode = Mode::COMPILATION
    );

    void analyze();
    void analyze(const std::vector<sir::Module *> &mods);
    void analyze(sir::Module &mod);

    ExtraAnalysis &get_extra_analysis() { return extra_analysis; }
    CompletionContext &get_completion_context() { return completion_context; }

private:
    Scope &get_scope() { return scopes.top(); }

    Scope &push_scope() {
        scopes.push(scopes.top());
        return get_scope();
    }

    Scope &push_empty_scope() {
        scopes.push({});
        return get_scope();
    }

    void pop_scope() { scopes.pop(); }

    void enter_mod(sir::Module *mod);
    void exit_mod() { scopes.pop(); }

    bool is_in_stmt_block() { return get_scope().block; }
    bool is_in_decl() { return get_scope().decl; }
    sir::SymbolTable &get_symbol_table();

    void populate_preamble_symbols();
    void run_postponed_analyses();

    sir::Symbol find_std_symbol(const ModulePath &mod_path, const std::string &name);
    sir::Symbol find_std_optional();
    sir::Symbol find_std_result();
    sir::Symbol find_std_array();
    sir::Symbol find_std_string();
    sir::Symbol find_std_map();
    sir::Symbol find_std_closure();

    sir::Specialization<sir::StructDef> *as_std_array_specialization(sir::Expr &type);
    sir::Specialization<sir::StructDef> *as_std_optional_specialization(sir::Expr &type);
    sir::Specialization<sir::StructDef> *as_std_result_specialization(sir::Expr &type);
    sir::Specialization<sir::StructDef> *as_std_map_specialization(sir::Expr &type);

    unsigned compute_size(sir::Expr type);

    void add_symbol_def(sir::Symbol sir_symbol);
    void add_symbol_use(ASTNode *ast_node, sir::Symbol sir_symbol);

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
