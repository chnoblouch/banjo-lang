#ifndef BANJO_SEMA_SEMANTIC_ANALYZER_H
#define BANJO_SEMA_SEMANTIC_ANALYZER_H

#include "banjo/reports/report_manager.hpp"
#include "banjo/sema/completion_context.hpp"
#include "banjo/sema/extra_analysis.hpp"
#include "banjo/sema/report_generator.hpp"
#include "banjo/sema/symbol_context.hpp"
#include "banjo/sir/sir.hpp"
#include "banjo/target/target.hpp"
#include "banjo/utils/typed_arena.hpp"

#include <cstddef>
#include <memory>
#include <set>
#include <stack>
#include <string_view>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
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

enum class Result {
    SUCCESS,
    ERROR,
    DEF_CYCLE,
};

enum class DeclStage {
    NAME,
    INTERFACE,
    BODY,
    RESOURCES,
};

struct DeclScope {
    sir::Module *mod;
    sir::Symbol decl = nullptr;
    sir::DeclBlock *decl_block = nullptr;
    ClosureContext *closure_ctx = nullptr;
};

struct DeclState {
    DeclStage stage;
    std::unique_ptr<DeclScope> scope;
};

struct Scope {
    DeclScope *decl_scope;
    sir::Block *block;
    sir::SymbolTable *symbol_table;
};

struct GuardedScope {
    unsigned guard_stmt_index;
    DeclScope &scope;
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

    std::unordered_map<std::string_view, sir::Symbol> preamble_symbols;
    std::unordered_map<std::string_view, sir::Expr> meta_field_types;

    DeclStage stage;

    sir::Module *mod;
    std::vector<sir::Decl> decl_stack;
    std::stack<Scope> scope_stack;

    std::vector<DeclState> decl_states;
    utils::TypedArena<ClosureContext> closure_ctxs;

    std::set<const sir::Decl *> blocked_decls;
    std::vector<GuardedScope> guarded_scopes;
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
    sir::Module &get_mod() { return *mod; }
    DeclScope &get_decl_scope() { return *scope_stack.top().decl_scope; }
    void enter_decl_scope(DeclScope &decl_scope);
    void exit_decl_scope();

    sir::Block &get_block() { return *scope_stack.top().block; }
    void enter_block(sir::Block &block);
    void exit_block() { scope_stack.pop(); }

    sir::SymbolTable &get_symbol_table() { return *scope_stack.top().symbol_table; }
    void enter_symbol_table(sir::SymbolTable &symbol_table);
    void exit_symbol_table() { scope_stack.pop(); }

    bool is_in_stmt_block() { return scope_stack.top().block; }
    bool is_in_decl() { return scope_stack.top().decl_scope->decl; }
    bool is_in_specialization();

    void populate_preamble_symbols();

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
        return get_mod().create(value);
    }

    template <typename T>
    std::span<T> allocate_array(unsigned length) {
        return get_mod().allocate_array<T>(length);
    }

    template <typename T>
    std::span<T> create_array(std::initializer_list<T> values) {
        return get_mod().create_array(std::move(values));
    }

    std::string_view create_string(std::string_view value) { return get_mod().create_string(value); }
};

std::strong_ordering operator<=>(const DeclStage &lhs, const DeclStage &rhs);

} // namespace sema

} // namespace lang

} // namespace banjo

#endif
