#include "banjo/ssa_gen/specialization_collector.hpp"

#include "banjo/sir/sir.hpp"
#include "banjo/sir/sir_visitor.hpp"
#include "banjo/sir/specializer.hpp"
#include "banjo/utils/arena.hpp"
#include "banjo/utils/macros.hpp"

#include <vector>

namespace banjo::lang {

SpecializationCollector::SpecializationCollector(utils::Arena<2048> &arena) : arena{arena} {}

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
    SIR_VISIT_STMT(
        stmt,
        SIR_VISIT_IGNORE,          // empty
        visit_var_stmt(*inner),    // var_stmt
        visit_assign_stmt(*inner), // assign_stmt
        SIR_VISIT_IMPOSSIBLE,      // comp_assign_stmt
        visit_return_stmt(*inner), // return_stmt
        visit_if_stmt(*inner),     // if_stmt
        visit_switch_stmt(*inner), // switch_stmt
        SIR_VISIT_IMPOSSIBLE,      // try_stmt
        SIR_VISIT_IMPOSSIBLE,      // while_stmt
        SIR_VISIT_IMPOSSIBLE,      // for_stmt
        visit_loop_stmt(*inner),   // loop_stmt
        SIR_VISIT_IGNORE,          // continue_stmt
        SIR_VISIT_IGNORE,          // break_stmt
        SIR_VISIT_IGNORE,          // meta_if_stmt
        SIR_VISIT_IGNORE,          // meta_for_stmt
        SIR_VISIT_IGNORE,          // expanded_meta_stmt
        visit_expr(*inner),        // expr_stmt
        visit_block(*inner),       // block_stmt
        SIR_VISIT_IMPOSSIBLE       // error
    );
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

void SpecializationCollector::visit_switch_stmt(const sir::SwitchStmt &switch_stmt) {
    visit_expr(switch_stmt.value);

    for (sir::SwitchCaseBranch &case_branch : switch_stmt.case_branches) {
        visit_expr(case_branch.local.type);
        visit_block(*case_branch.block);
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
    SIR_VISIT_EXPR(
        expr,
        SIR_VISIT_IGNORE,                // empty
        SIR_VISIT_IGNORE,                // int_literal (TODO)
        SIR_VISIT_IGNORE,                // fp_literal (TODO)
        SIR_VISIT_IGNORE,                // bool_literal (TODO)
        SIR_VISIT_IGNORE,                // char_literal (TODO)
        SIR_VISIT_IGNORE,                // null_literal (TODO)
        SIR_VISIT_IMPOSSIBLE,            // none_literal
        SIR_VISIT_IGNORE,                // undefined_literal (TODO)
        SIR_VISIT_IGNORE,                // array_literal (TODO)
        SIR_VISIT_IGNORE,                // string_literal (TODO)
        visit_struct_literal(*inner),    // struct_literal
        SIR_VISIT_IGNORE,                // union_case_literal (TODO)
        SIR_VISIT_IMPOSSIBLE,            // map_literal
        SIR_VISIT_IMPOSSIBLE,            // closure_literal
        SIR_VISIT_IGNORE,                // symbol_expr (TODO)
        visit_binary_expr(*inner),       // binary_expr
        visit_unary_expr(*inner),        // unary_expr
        visit_cast_expr(*inner),         // cast_expr
        visit_index_expr(*inner),        // index_expr
        visit_call_expr(*inner),         // call_expr
        visit_field_expr(*inner),        // field_expr
        visit_range_expr(*inner),        // range_expr
        visit_try_expr(*inner),          // try_expr
        visit_tuple_expr(*inner),        // tuple_expr
        visit_coercion_expr(*inner),     // coercion_expr
        visit_specialize_expr(*inner),   // specialize_expr
        SIR_VISIT_IGNORE,                // primitive_type
        visit_pointer_type(*inner),      // pointer_type
        visit_static_array_type(*inner), // static_array_type
        visit_func_type(*inner),         // func_type
        SIR_VISIT_IMPOSSIBLE,            // optional_type
        SIR_VISIT_IMPOSSIBLE,            // result_type
        SIR_VISIT_IMPOSSIBLE,            // array_type
        SIR_VISIT_IMPOSSIBLE,            // map_type
        SIR_VISIT_IMPOSSIBLE,            // closure_type
        visit_reference_type(*inner),    // reference_type
        SIR_VISIT_IMPOSSIBLE,            // ident_expr
        SIR_VISIT_IMPOSSIBLE,            // star_expr
        SIR_VISIT_IMPOSSIBLE,            // bracket_expr
        SIR_VISIT_IMPOSSIBLE,            // dot_expr
        SIR_VISIT_IMPOSSIBLE,            // pseudo_type
        visit_meta_access(*inner),       // meta_access
        visit_meta_field_expr(*inner),   // meta_field_expr
        visit_meta_call_expr(*inner),    // meta_call_expr
        visit_init_expr(*inner),         // init_expr
        visit_move_expr(*inner),         // move_expr
        visit_deinit_expr(*inner),       // deinit_expr
        SIR_VISIT_IGNORE,                // type_guard_expr (TODO)
        SIR_VISIT_IGNORE,                // placeholder_expr (TODO)
        SIR_VISIT_IMPOSSIBLE             // error
    )
}

void SpecializationCollector::visit_struct_literal(const sir::StructLiteral &struct_literal) {
    visit_expr(struct_literal.type);

    for (sir::StructLiteralEntry &entry : struct_literal.entries) {
        visit_expr(entry.value);
    }
}

void SpecializationCollector::visit_binary_expr(const sir::BinaryExpr &binary_expr) {
    visit_expr(binary_expr.type);
    visit_expr(binary_expr.lhs);
    visit_expr(binary_expr.rhs);
}

void SpecializationCollector::visit_unary_expr(const sir::UnaryExpr &unary_expr) {
    visit_expr(unary_expr.type);
    visit_expr(unary_expr.value);
}

void SpecializationCollector::visit_cast_expr(const sir::CastExpr &cast_expr) {
    visit_expr(cast_expr.type);
    visit_expr(cast_expr.value);
}

void SpecializationCollector::visit_index_expr(const sir::IndexExpr &index_expr) {
    visit_expr(index_expr.type);
    visit_expr(index_expr.base);
    visit_expr(index_expr.index);
}

void SpecializationCollector::visit_call_expr(const sir::CallExpr &call_expr) {
    visit_expr(call_expr.type);
    visit_expr(call_expr.callee);

    for (sir::Expr arg : call_expr.args) {
        visit_expr(arg);
    }
}

void SpecializationCollector::visit_field_expr(const sir::FieldExpr &field_expr) {
    visit_expr(field_expr.type);
    visit_expr(field_expr.base);
}

void SpecializationCollector::visit_range_expr(const sir::RangeExpr &range_expr) {
    visit_expr(range_expr.lhs);
    visit_expr(range_expr.rhs);
}

void SpecializationCollector::visit_try_expr(const sir::TryExpr &try_expr) {
    // FIXME: Implicit specializations created here.
    
    visit_expr(try_expr.type);
    visit_expr(try_expr.value);
    visit_stmt(try_expr.return_stmt);
}

void SpecializationCollector::visit_tuple_expr(const sir::TupleExpr &tuple_expr) {
    visit_expr(tuple_expr.type);

    for (sir::Expr expr : tuple_expr.exprs) {
        visit_expr(expr);
    }
}

void SpecializationCollector::visit_coercion_expr(const sir::CoercionExpr &coercion_expr) {
    visit_expr(coercion_expr.type);
    visit_expr(coercion_expr.value);
}

void SpecializationCollector::visit_specialize_expr(const sir::SpecializeExpr &specialize_expr) {
    std::vector<sir::Expr> args(specialize_expr.args.begin(), specialize_expr.args.end());

    if (!entry_stack.empty()) {
        SpecializationCollector::Entry &entry = entry_stack.back();

        sir::Specializer specializer{arena, entry.params, entry.args};
        std::span<sir::Expr> specialized_args = specializer.specialize_expr_list(args);
        args.assign(specialized_args.begin(), specialized_args.end());
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

void SpecializationCollector::visit_pointer_type(const sir::PointerType &pointer_type) {
    visit_expr(pointer_type.base_type);
}

void SpecializationCollector::visit_static_array_type(const sir::StaticArrayType &static_array_type) {
    visit_expr(static_array_type.base_type);
    visit_expr(static_array_type.length);
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

void SpecializationCollector::visit_init_expr(const sir::InitExpr &init_expr) {
    visit_expr(init_expr.type);
    visit_expr(init_expr.value);
}

void SpecializationCollector::visit_move_expr(const sir::MoveExpr &move_expr) {
    visit_expr(move_expr.type);
    visit_expr(move_expr.value);
}

void SpecializationCollector::visit_deinit_expr(const sir::DeinitExpr &deinit_expr) {
    visit_expr(deinit_expr.type);
    visit_expr(deinit_expr.value);
}

} // namespace banjo::lang
