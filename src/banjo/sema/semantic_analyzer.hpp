#ifndef BANJO_SEMA_SEMANTIC_ANALYZER_H
#define BANJO_SEMA_SEMANTIC_ANALYZER_H

#include "banjo/reports/report_manager.hpp"
#include "banjo/sema/completion_context.hpp"
#include "banjo/sema/extra_analysis.hpp"
#include "banjo/sema/report_generator.hpp"
#include "banjo/sema/symbol_context.hpp"
#include "banjo/sir/sir.hpp"
#include "banjo/target/target.hpp"

#include <cstddef>
#include <set>
#include <stack>
#include <string_view>
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

enum class Result {
    SUCCESS,
    ERROR,
    DEF_CYCLE,
};

struct Scope {
    sir::Symbol decl;
    sir::DeclBlock *decl_block;
    sir::Block *block;
    sir::SymbolTable *symbol_table;
    ClosureContext *closure_ctx;
};

struct GuardedScope {
    unsigned guard_stmt_index;
    sir::Symbol decl;
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

public:
    ReportGenerator report_generator;

private:
    Mode mode;

    ExtraAnalysis extra_analysis;
    CompletionContext completion_context;
    CompletionInfection completion_infection;

    sir::StructDef *std_result_def;
    std::unordered_map<std::string_view, sir::Symbol> preamble_symbols;
    std::unordered_map<std::string_view, sir::Expr> meta_field_types;

    sir::SemaStage stage;

    sir::Module *mod;
    std::vector<sir::Decl> decl_stack;
    std::stack<Scope> scope_stack;

    std::set<const sir::Decl *> blocked_decls;
    std::vector<GuardedScope> guarded_scopes;
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
    sir::DeclBlock &get_decl_block() { return *scope_stack.top().decl_block; }
    sir::SymbolTable &get_symbol_table() { return *scope_stack.top().symbol_table; }

    sir::Symbol get_decl() { return scope_stack.top().decl; }
    void enter_decl(sir::Symbol decl);
    void exit_decl();

    sir::Block &get_block() { return *scope_stack.top().block; }
    void enter_block(sir::Block &block);
    void exit_block() { scope_stack.pop(); }

    ClosureContext *get_closure_ctx() { return scope_stack.top().closure_ctx; }
    void enter_closure_ctx(ClosureContext &closure_ctx);
    void exit_closure_ctx() { scope_stack.pop(); }

    bool is_in_stmt_block() { return scope_stack.top().block; }
    bool is_in_decl() { return !is_in_stmt_block(); }
    bool is_in_specialization();

    void populate_preamble_symbols();

    sir::Symbol find_std_symbol(const ModulePath &mod_path, const std::string &name);
    sir::Symbol find_std_optional();
    sir::StructDef &get_std_result() { return *std_result_def; }
    sir::Symbol find_std_array();
    sir::Symbol find_std_string();
    sir::Symbol find_std_string_slice();
    sir::Symbol find_std_map();
    sir::Symbol find_std_closure();

    sir::Specialization<sir::StructDef> *as_std_array_specialization(sir::Expr &type);
    sir::Specialization<sir::StructDef> *as_std_optional_specialization(sir::Expr &type);
    sir::Specialization<sir::StructDef> *as_std_result_specialization(sir::Expr &type);
    sir::Specialization<sir::StructDef> *as_std_map_specialization(sir::Expr &type);

    Result ensure_interface_analyzed(sir::Symbol symbol, ASTNode *ident_ast_node);
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

} // namespace sema

} // namespace lang

} // namespace banjo

#endif
