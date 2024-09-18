#ifndef SEMA2_SEMANTIC_ANALYZER_H
#define SEMA2_SEMANTIC_ANALYZER_H

#include "banjo/reports/report_manager.hpp"
#include "banjo/sema2/report_generator.hpp"
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

struct ClosureContext {
    std::vector<sir::Symbol> captured_vars;
    sir::TupleExpr *data_type;
    sir::Block *parent_block;
};

struct Scope {
    sir::FuncDef *func_def = nullptr;
    sir::Block *block = nullptr;
    sir::DeclBlock *decl_block = nullptr;
    sir::StructDef *struct_def = nullptr;
    sir::EnumDef *enum_def = nullptr;
    sir::UnionDef *union_def = nullptr;
    std::unordered_map<std::string_view, sir::Expr> generic_args;
    ClosureContext *closure_ctx = nullptr;
};

enum class Result {
    SUCCESS,
    ERROR,
    AWAITING_DEPENDENCY,
};

class SemanticAnalyzer {

    // This is getting slightly out of hand...
    friend class SymbolCollector;
    friend class UseResolver;
    friend class TypeAliasResolver;
    friend class DeclInterfaceAnalyzer;
    friend class DeclValueAnalyzer;
    friend class DeclBodyAnalyzer;
    friend class ExprAnalyzer;
    friend class StmtAnalyzer;
    friend class ConstEvaluator;
    friend class GenericsSpecializer;
    friend class GenericArgInference;
    friend class MetaExpansion;
    friend class MetaExprEvaluator;
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
    std::set<const sir::Decl *> blocked_decls;
    std::vector<std::tuple<sir::Decl, Scope>> decls_awaiting_body_analysis;
    bool in_meta_expansion = false;

public:
    SemanticAnalyzer(sir::Unit &sir_unit, target::Target *target, ReportManager &report_manager);
    void analyze();

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
    void enter_struct_def(sir::StructDef *struct_def);
    void exit_struct_def() { scopes.pop(); }
    void enter_enum_def(sir::EnumDef *enum_def);
    void exit_enum_def() { scopes.pop(); }
    void enter_union_def(sir::UnionDef *union_def);
    void exit_union_def() { scopes.pop(); }

    bool is_in_stmt_block() { return get_scope().block; }
    sir::SymbolTable &get_symbol_table();

    sir::Symbol find_std_symbol(const ModulePath &mod_path, const std::string &name);
    sir::Symbol find_std_optional();
    sir::Symbol find_std_result();
    sir::Symbol find_std_array();
    sir::Symbol find_std_string();
    sir::Symbol find_std_closure();

    sir::Specialization<sir::StructDef> *as_std_array_specialization(sir::Expr &type);
    sir::Specialization<sir::StructDef> *as_std_optional_specialization(sir::Expr &type);
    sir::Specialization<sir::StructDef> *as_std_result_specialization(sir::Expr &type);

    unsigned compute_size(sir::Expr type);

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
