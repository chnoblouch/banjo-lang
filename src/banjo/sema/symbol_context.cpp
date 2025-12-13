#include "symbol_context.hpp"

#include "banjo/sema/meta_expansion.hpp"
#include "banjo/sema/semantic_analyzer.hpp"
#include "banjo/sir/sir.hpp"
#include <optional>

namespace banjo::lang::sema {

SymbolContext::SymbolContext(SemanticAnalyzer &analyzer) : analyzer(analyzer) {}

void SymbolContext::push_meta_condition() {
    sub_meta_conditions.push_back(sir::GuardedSymbol::TruthTable{});
}

void SymbolContext::add_next_meta_condition(sir::Expr expr) {
    sir::GuardedSymbol::TruthTable prev_negated = sub_meta_conditions.back().negate();
    sub_meta_conditions.back() = sir::GuardedSymbol::TruthTable::merge_and(prev_negated, build_condition(expr));
    meta_condition = sir::GuardedSymbol::TruthTable::merge_and(sub_meta_conditions);
}

void SymbolContext::add_else_meta_condition() {
    sub_meta_conditions.back() = sub_meta_conditions.back().negate();
    meta_condition = sir::GuardedSymbol::TruthTable::merge_and(sub_meta_conditions);
}

void SymbolContext::pop_meta_condition() {
    sub_meta_conditions.pop_back();
    meta_condition = sir::GuardedSymbol::TruthTable::merge_and(sub_meta_conditions);
}

SymbolLookupResult SymbolContext::look_up(const sir::IdentExpr &ident_expr) {
    sir::SymbolTable &symbol_table = analyzer.get_symbol_table();
    ClosureContext *closure_ctx = analyzer.get_decl_scope().closure_ctx;

    std::string_view name = ident_expr.value;

    SymbolLookupResult result{
        .kind = SymbolLookupResult::Kind::SUCCESS,
        .guarded = false,
        .symbol = symbol_table.look_up(name),
    };

    if (!result.symbol && closure_ctx) {
        result.symbol = closure_ctx->parent_block->symbol_table->look_up(name);

        if (result.symbol) {
            result.kind = SymbolLookupResult::Kind::CAPTURED;
            result.closure_ctx = closure_ctx;
        }
    }

    if (!result.symbol) {
        auto iter = analyzer.preamble_symbols.find(name);
        result.symbol = iter == analyzer.preamble_symbols.end() ? nullptr : iter->second;
    }

    if (!result.symbol) {
        auto iter = symbol_table.guarded_scopes.find(name);
        if (iter != symbol_table.guarded_scopes.end()) {
            GuardedScope &guarded_scope = analyzer.guarded_scopes[iter->second];
            sir::DeclBlock &block = *guarded_scope.scope.decl_block;
            unsigned guard_stmt_index = guarded_scope.guard_stmt_index;

            analyzer.enter_decl_scope(guarded_scope.scope);
            MetaExpansion(analyzer).evaluate_meta_if_stmt(block, guard_stmt_index);
            analyzer.exit_decl_scope();

            result.symbol = symbol_table.look_up(name);
        }
    }

    if (!result.symbol) {
        analyzer.report_generator.report_err_symbol_not_found(ident_expr);

        return SymbolLookupResult{
            .kind = SymbolLookupResult::Kind::ERROR,
            .guarded = false,
            .symbol = nullptr,
        };
    }

    return resolve_if_guarded(result, ident_expr.ast_node);
}

SymbolLookupResult SymbolContext::look_up_rhs_local(sir::DotExpr &dot_expr) {
    sir::DeclBlock *decl_block = dot_expr.lhs.get_decl_block();
    return look_up_rhs_local(dot_expr, *decl_block->symbol_table);
}

SymbolLookupResult SymbolContext::look_up_rhs_local(sir::DotExpr &dot_expr, sir::SymbolTable &symbol_table) {
    std::string_view name = dot_expr.rhs.value;

    SymbolLookupResult result{
        .kind = SymbolLookupResult::Kind::SUCCESS,
        .symbol = symbol_table.look_up_local(name),
        .closure_ctx = nullptr,
    };

    return resolve_if_guarded(result, dot_expr.rhs.ast_node);
}

sir::GuardedSymbol::TruthTable SymbolContext::build_condition(sir::Expr expr) {
    if (auto binary_expr = expr.match<sir::BinaryExpr>()) {
        return build_condition(*binary_expr);
    } else if (auto unary_expr = expr.match<sir::UnaryExpr>()) {
        return build_condition(*unary_expr);
    } else {
        return sir::GuardedSymbol::TruthTable{expr};
    }
}

sir::GuardedSymbol::TruthTable SymbolContext::build_condition(sir::BinaryExpr &binary_expr) {
    if (binary_expr.op == sir::BinaryOp::AND) {
        sir::GuardedSymbol::TruthTable lhs = build_condition(binary_expr.lhs);
        sir::GuardedSymbol::TruthTable rhs = build_condition(binary_expr.rhs);
        return sir::GuardedSymbol::TruthTable::merge_and(lhs, rhs);
    } else if (binary_expr.op == sir::BinaryOp::OR) {
        sir::GuardedSymbol::TruthTable lhs = build_condition(binary_expr.lhs);
        sir::GuardedSymbol::TruthTable rhs = build_condition(binary_expr.rhs);
        return sir::GuardedSymbol::TruthTable::merge_or(lhs, rhs);
    } else {
        return sir::GuardedSymbol::TruthTable{&binary_expr};
    }
}

sir::GuardedSymbol::TruthTable SymbolContext::build_condition(sir::UnaryExpr &unary_expr) {
    if (unary_expr.op == sir::UnaryOp::NOT) {
        return build_condition(unary_expr.value).negate();
    } else {
        return sir::GuardedSymbol::TruthTable{&unary_expr};
    }
}

SymbolLookupResult SymbolContext::resolve_if_guarded(SymbolLookupResult result, ASTNode *ast_node) {
    if (auto guarded_symbol = result.symbol.match<sir::GuardedSymbol>()) {
        for (sir::GuardedSymbol::Variant &variant : guarded_symbol->variants) {
            if (variant.truth_table.is_subset_of(meta_condition)) {
                return SymbolLookupResult{
                    .kind = result.kind,
                    .guarded = true,
                    .symbol = variant.symbol,
                };
            }
        }

        analyzer.report_generator.report_err_symbol_guarded(ast_node, *guarded_symbol);

        return SymbolLookupResult{
            .kind = SymbolLookupResult::Kind::ERROR,
            .guarded = true,
            .symbol = nullptr,
        };
    } else {
        return result;
    }
}

} // namespace banjo::lang::sema
