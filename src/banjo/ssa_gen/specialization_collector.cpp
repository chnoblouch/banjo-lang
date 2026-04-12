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
        } else if (auto struct_def = decl.match<sir::StructDef>()) {
            visit_struct_def(*struct_def);
        }
    }
}

void SpecializationCollector::visit_func_def(const sir::FuncDef &func_def, bool is_specialized /* = false */) {
    if (func_def.is_generic() && !is_specialized) {
        return;
    }

    visit_func_type(func_def.type);
    visit_block(func_def.block);
}

void SpecializationCollector::visit_struct_def(const sir::StructDef &struct_def, bool is_specialized /* = false */) {
    if (struct_def.is_generic() && !is_specialized) {
        return;
    }

    visit_decl_block(struct_def.block);
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
    } else if (auto loop_stmt = stmt.match<sir::LoopStmt>()) {
        visit_loop_stmt(*loop_stmt);
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

void SpecializationCollector::visit_loop_stmt(const sir::LoopStmt &loop_stmt) {
    visit_expr(loop_stmt.condition);
    visit_block(*loop_stmt.block);

    if (loop_stmt.latch) {
        visit_block(*loop_stmt.latch);
    }
}

void SpecializationCollector::visit_expr(sir::Expr expr) {
    if (auto unary_expr = expr.match<sir::UnaryExpr>()) {
        visit_unary_expr(*unary_expr);
    } else if (auto call_expr = expr.match<sir::CallExpr>()) {
        visit_call_expr(*call_expr);
    } else if (auto specialize_expr = expr.match<sir::SpecializeExpr>()) {
        visit_specialize_expr(*specialize_expr);
    } else if (auto func_type = expr.match<sir::FuncType>()) {
        visit_func_type(*func_type);
    } else if (auto reference_type = expr.match<sir::ReferenceType>()) {
        visit_reference_type(*reference_type);
    } else if (auto meta_access = expr.match<sir::MetaAccess>()) {
        visit_meta_access(*meta_access);
    } else if (auto meta_field_expr = expr.match<sir::MetaFieldExpr>()) {
        visit_meta_field_expr(*meta_field_expr);
    } else if (auto meta_call_expr = expr.match<sir::MetaCallExpr>()) {
        visit_meta_call_expr(*meta_call_expr);
    } else if (auto init_expr = expr.match<sir::InitExpr>()) {
        visit_expr(init_expr->value);
    } else if (auto move_expr = expr.match<sir::MoveExpr>()) {
        visit_expr(move_expr->value);
    } else if (auto deinit_expr = expr.match<sir::DeinitExpr>()) {
        visit_expr(deinit_expr->value);
    }
}

void SpecializationCollector::visit_unary_expr(const sir::UnaryExpr &unary_expr) {
    visit_expr(unary_expr.type);
    visit_expr(unary_expr.value);
}

void SpecializationCollector::visit_call_expr(const sir::CallExpr &call_expr) {
    visit_expr(call_expr.type);
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

    std::span<sir::GenericParam *> generic_params;

    if (auto func_def = specialize_expr.symbol.match<sir::FuncDef>()) {
        generic_params = func_def->generic_params;
    } else if (auto struct_def = specialize_expr.symbol.match<sir::StructDef>()) {
        generic_params = struct_def->generic_params;
    } else {
        ASSERT_UNREACHABLE;
    }

    Entry entry{
        .params = generic_params,
        .args = args,
    };

    entries.push_back(entry);
    entry_stack.push_back(entry);

    if (auto func_def = specialize_expr.symbol.match<sir::FuncDef>()) {
        visit_func_def(*func_def, true);
    } else if (auto struct_def = specialize_expr.symbol.match<sir::StructDef>()) {
        visit_struct_def(*struct_def, true);
    } else {
        ASSERT_UNREACHABLE;
    }

    entry_stack.pop_back();
}

void SpecializationCollector::visit_func_type(const sir::FuncType &func_type) {    
    for (sir::Param &arg : func_type.params) {
        visit_expr(arg.type);
    }

    visit_expr(func_type.return_type);
}

void SpecializationCollector::visit_reference_type(const sir::ReferenceType &reference_type) {
    visit_expr(reference_type.base_type);
}

void SpecializationCollector::visit_meta_access(const sir::MetaAccess &meta_access) {
    visit_expr(meta_access.expr);
}

void SpecializationCollector::visit_meta_field_expr(const sir::MetaFieldExpr &meta_field_expr) {
    visit_expr(meta_field_expr.type);
    visit_expr(meta_field_expr.base);
}

void SpecializationCollector::visit_meta_call_expr(const sir::MetaCallExpr &meta_call_expr) {
    visit_expr(meta_call_expr.type);
    visit_expr(meta_call_expr.callee);

    for (sir::Expr arg : meta_call_expr.args) {
        visit_expr(arg);
    }
}

} // namespace banjo::lang
