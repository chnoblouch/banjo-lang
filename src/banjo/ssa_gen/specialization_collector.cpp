#include "banjo/ssa_gen/specialization_collector.hpp"
#include "banjo/sir/sir.hpp"
#include "banjo/sir/specializer.hpp"
#include "banjo/utils/arena.hpp"
#include <vector>

namespace banjo::lang {

SpecializationCollector::Map SpecializationCollector::collect(const sir::Unit &unit) {
    for (const sir::Module *mod : unit.mods) {
        visit_decl_block(mod->block);
    }

    return std::move(specializations);
}

void SpecializationCollector::visit_decl_block(const sir::DeclBlock &decl_block) {
    for (sir::Decl decl : decl_block.decls) {
        if (auto func_def = decl.match<sir::FuncDef>()) {
            visit_func_def(*func_def);
        }
    }
}

void SpecializationCollector::visit_func_def(const sir::FuncDef &func_def) {
    if (func_def.is_generic()) {
        bool is_specialized = false;

        for (Entry &entry : entry_stack) {
            if (&entry.params == &func_def.generic_params) {
                is_specialized = true;
            }
        }

        if (!is_specialized) {
            return;
        }
    }

    visit_block(func_def.block);
}

void SpecializationCollector::visit_block(const sir::Block &block) {
    for (sir::Stmt stmt : block.stmts) {
        visit_stmt(stmt);
    }
}

void SpecializationCollector::visit_stmt(sir::Stmt stmt) {
    if (auto expr = stmt.match<sir::Expr>()) {
        visit_expr(*expr);
    }
}

void SpecializationCollector::visit_expr(sir::Expr expr) {
    if (auto call_expr = expr.match<sir::CallExpr>()) {
        visit_call_expr(*call_expr);
    } else if (auto specialize_expr = expr.match<sir::SpecializeExpr>()) {
        visit_specialize_expr(*specialize_expr);
    }
}

void SpecializationCollector::visit_call_expr(const sir::CallExpr &call_expr) {
    visit_expr(call_expr.callee);

    for (sir::Expr arg : call_expr.args) {
        visit_expr(arg);
    }
}

void SpecializationCollector::visit_specialize_expr(const sir::SpecializeExpr &specialize_expr) {
    std::vector<sir::Expr> args(specialize_expr.args.begin(), specialize_expr.args.end());

    if (!entry_stack.empty()) {
        utils::Arena<2048> arena;

        for (Entry &entry : entry_stack) {
            sir::Specializer specializer{arena, entry.params, entry.args};
            std::span<sir::Expr> specialized_args = specializer.specialize_expr_list(args);
            args.assign(specialized_args.begin(), specialized_args.end());
        }
    }

    std::vector<Entry> &entries = specializations[specialize_expr.symbol];

    for (Entry &entry : entries) {
        if (entry.args == args) {
            return;
        }
    }

    Entry entry{
        .params = specialize_expr.symbol.as<sir::FuncDef>().generic_params,
        .args = args,
    };

    entries.push_back(entry);

    entry_stack.push_back(entry);
    visit_func_def(specialize_expr.symbol.as<sir::FuncDef>());
    entry_stack.pop_back();
}

} // namespace banjo::lang
