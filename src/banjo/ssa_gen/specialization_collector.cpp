#include "banjo/ssa_gen/specialization_collector.hpp"

#include "banjo/sir/sir.hpp"
#include "banjo/sir/specializer.hpp"
#include "banjo/utils/arena.hpp"
#include "banjo/utils/macros.hpp"

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
    // TODO

    if (auto var_stmt = stmt.match<sir::VarStmt>()) {
        visit_var_stmt(*var_stmt);
    } else if (auto assign_stmt = stmt.match<sir::AssignStmt>()) {
        visit_assign_stmt(*assign_stmt);
    } else if (auto return_stmt = stmt.match<sir::ReturnStmt>()) {
        visit_return_stmt(*return_stmt);
    } else if (auto if_stmt = stmt.match<sir::IfStmt>()) {
        visit_if_stmt(*if_stmt);
    } else if (auto expr = stmt.match<sir::Expr>()) {
        visit_expr(*expr);
    } else if (auto block = stmt.match<sir::Block>()) {
        visit_block(*block);
    }
}

void SpecializationCollector::visit_var_stmt(const sir::VarStmt &var_stmt) {
    visit_expr(var_stmt.local.type);
    visit_expr(var_stmt.value);
}

void SpecializationCollector::visit_assign_stmt(const sir::AssignStmt &assign_stmt) {
    visit_expr(assign_stmt.lhs);
    visit_expr(assign_stmt.rhs);
}

void SpecializationCollector::visit_return_stmt(const sir::ReturnStmt &return_stmt) {
    visit_expr(return_stmt.value);
}

void SpecializationCollector::visit_if_stmt(const sir::IfStmt &if_stmt) {
    for (const sir::IfCondBranch &cond_branch : if_stmt.cond_branches) {
        visit_expr(cond_branch.condition);
        visit_block(*cond_branch.block);
    }

    if (if_stmt.else_branch) {
        visit_block(*if_stmt.else_branch->block);
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

    const std::vector<sir::GenericParam> *generic_params;

    if (auto func_def = specialize_expr.symbol.match<sir::FuncDef>()) {
        generic_params = &func_def->generic_params;
    } else if (auto struct_def = specialize_expr.symbol.match<sir::StructDef>()) {
        generic_params = &struct_def->generic_params;
    } else {
        ASSERT_UNREACHABLE;
    }

    Entry entry{
        .params = *generic_params,
        .args = args,
    };

    entries.push_back(entry);

    if (auto func_def = specialize_expr.symbol.match<sir::FuncDef>()) {
        entry_stack.push_back(entry);
        visit_func_def(*func_def);
        entry_stack.pop_back();
    }
}

} // namespace banjo::lang
