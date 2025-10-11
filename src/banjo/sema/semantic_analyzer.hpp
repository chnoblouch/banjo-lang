#ifndef BANJO_SEMA_SEMANTIC_ANALYZER_H
#define BANJO_SEMA_SEMANTIC_ANALYZER_H

#include "banjo/reports/report_manager.hpp"
#include "banjo/sema/completion_context.hpp"
#include "banjo/sema/extra_analysis.hpp"
#include "banjo/sema/report_generator.hpp"
#include "banjo/sema/symbol_context.hpp"
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

struct CompletionInfection {
    std::unordered_map<sir::FuncDef *, unsigned> func_specializations;
    std::unordered_map<sir::StructDef *, unsigned> struct_specializations;
};

class SemanticAnalyzer {

    // This is getting slightly out of hand...
    friend class SymbolCollector;
    friend class UseResolver;
    friend class TypeAliasResolver;
    friend class DeclInterfaceAnalyzer;
    friend class DeclValueAnalyzer;
    friend class DeclBodyAnalyzer;
    friend class ReturnChecker;
    friend class PointerEscapeChecker;
    friend class ResourceAnalyzer;
    friend class ExprAnalyzer;
    friend class ExprFinalizer;
    friend class StmtAnalyzer;
    friend class OverloadResolver;
    friend class ConstEvaluator;
    friend class GenericsSpecializer;
    friend class GenericArgInference;
    friend class MetaExpansion;
    friend class MetaExprEvaluator;
    friend class DeclVisitor;
    friend class ReportGenerator;
    friend class ReportBuilder;
    friend class SymbolContext;

public:
    SymbolContext symbol_ctx;

private:
    sir::Unit &sir_unit;
    target::Target *target;
    ReportManager &report_manager;
    ReportGenerator report_generator;
    Mode mode;

    ExtraAnalysis extra_analysis;
    CompletionContext completion_context;
    CompletionInfection completion_infection;

    sir::Module *cur_sir_mod;
    std::stack<Scope> scopes;
    std::unordered_map<std::string_view, sir::Symbol> preamble_symbols;
    std::unordered_map<std::string_view, sir::Expr> meta_field_types;
    std::set<const sir::Decl *> blocked_decls;
    std::vector<std::tuple<sir::Decl, Scope>> decls_awaiting_body_analysis;
    std::unordered_map<sir::DeclBlock *, Scope> incomplete_decl_blocks;
    bool in_meta_expansion = false;
    unsigned loop_depth = 0;

public:
    std::vector<sir::Expr> meta_conditions;

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
    CompletionInfection &get_completion_infection() { return completion_infection; }
    const std::unordered_map<std::string_view, sir::Symbol> &get_preamble_symbols() { return preamble_symbols; }

private:
    Scope &get_scope() { return scopes.top(); }

    Scope &push_scope() {
        scopes.push(scopes.top());
        return get_scope();
    }

    Scope &push_empty_scope() {
        scopes.push(Scope{});
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
    sir::Symbol find_std_string_slice();
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
    T *create(T value) {
        return cur_sir_mod->create(value);
    }

    template <typename T>
    std::span<T> allocate_array(unsigned length) {
        return cur_sir_mod->allocate_array<T>(length);
    }

    template <typename T>
    std::span<T> create_array(std::initializer_list<T> values) {
        return cur_sir_mod->create_array(std::move(values));
    }

    std::string_view create_string(std::string_view value) { return cur_sir_mod->create_string(value); }
};

} // namespace sema

} // namespace lang

} // namespace banjo

#endif
