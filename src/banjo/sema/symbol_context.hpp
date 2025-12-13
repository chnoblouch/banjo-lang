#ifndef BANJO_SEMA_SYMBOL_CONTEXT_H
#define BANJO_SEMA_SYMBOL_CONTEXT_H

#include "banjo/sir/sir.hpp"

#include <vector>

namespace banjo::lang::sema {

class SemanticAnalyzer;
struct ClosureContext;

struct SymbolLookupResult {
    enum class Kind {
        SUCCESS,
        ERROR,
        CAPTURED,
    };

    Kind kind;
    bool guarded;
    sir::Symbol symbol;
    ClosureContext *closure_ctx = nullptr;
};

class SymbolContext {

private:
    SemanticAnalyzer &analyzer;
    sir::GuardedSymbol::TruthTable meta_condition;
    std::vector<sir::GuardedSymbol::TruthTable> sub_meta_conditions;

public:
    SymbolContext(SemanticAnalyzer &analyzer);

    void push_meta_condition();
    void add_next_meta_condition(sir::Expr expr);
    void add_else_meta_condition();
    void pop_meta_condition();

    SymbolLookupResult look_up(const sir::IdentExpr &ident_expr);
    SymbolLookupResult look_up_rhs_local(sir::DotExpr &dot_expr);
    SymbolLookupResult look_up_rhs_local(sir::DotExpr &dot_expr, sir::SymbolTable &symbol_table);

    sir::GuardedSymbol::TruthTable get_meta_condition() { return meta_condition; }
    bool is_guarded() { return !sub_meta_conditions.empty(); }

private:
    sir::GuardedSymbol::TruthTable build_condition(sir::Expr expr);
    sir::GuardedSymbol::TruthTable build_condition(sir::BinaryExpr &binary_expr);
    sir::GuardedSymbol::TruthTable build_condition(sir::UnaryExpr &unary_expr);

    SymbolLookupResult resolve_if_guarded(SymbolLookupResult result, ASTNode *ast_node);
};

} // namespace banjo::lang::sema

#endif
