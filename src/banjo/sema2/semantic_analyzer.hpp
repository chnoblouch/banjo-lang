#ifndef SEMA2_SEMANTIC_ANALYZER_H
#define SEMA2_SEMANTIC_ANALYZER_H

#include "banjo/sir/sir.hpp"
#include "banjo/target/target.hpp"

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
    sir::SymbolTable *symbol_table = nullptr;
    sir::StructDef *struct_def = nullptr;
    std::unordered_map<std::string_view, sir::Expr> generic_args;
};

class SemanticAnalyzer {

    friend class SymbolCollector;
    friend class UseResolver;
    friend class DeclInterfaceAnalyzer;
    friend class DeclBodyAnalyzer;
    friend class ExprAnalyzer;
    friend class StmtAnalyzer;
    friend class GenericsSpecializer;

private:
    sir::Unit &sir_unit;
    target::Target *target;
    sir::Module *cur_sir_mod;
    std::stack<Scope> scopes;

public:
    SemanticAnalyzer(sir::Unit &sir_unit, target::Target *target);
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

    sir::SymbolTable *create_symbol_table(sir::SymbolTable value) {
        return cur_sir_mod->create_symbol_table(std::move(value));
    }
};

} // namespace sema

} // namespace lang

} // namespace banjo

#endif
