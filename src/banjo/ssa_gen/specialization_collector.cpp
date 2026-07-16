#include "banjo/ssa_gen/specialization_collector.hpp"

#include "banjo/sir/magic_methods.hpp"
#include "banjo/sir/resource_generator.hpp"
#include "banjo/sir/sir.hpp"
#include "banjo/sir/sir_visitor.hpp"
#include "banjo/sir/specializer.hpp"
#include "banjo/utils/arena.hpp"
#include "banjo/utils/macros.hpp"

#include <vector>

namespace banjo::lang {

sir::Expr SpecializationCollector::Entry::resolve_param(const sir::GenericParam &param) {
    for (unsigned i = 0; i < params.size(); i++) {
        if (params[i] == &param) {
            return args[i];
        }
    }

    ASSERT_UNREACHABLE;
}

SpecializationCollector::SpecializationCollector(utils::Arena &arena) : arena{arena} {}

SpecializationCollector::List SpecializationCollector::collect(const sir::Unit &unit) {
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

    for (const auto &[symbol, resource] : block.resources) {
        visit_resource(resource, true);
    }
}

void SpecializationCollector::visit_stmt(sir::Stmt stmt) {
    SIR_VISIT_STMT(
        stmt,
        SIR_VISIT_IGNORE,            // empty
        visit_var_stmt(*inner),      // var_stmt
        visit_assign_stmt(*inner),   // assign_stmt
        SIR_VISIT_IMPOSSIBLE,        // comp_assign_stmt
        visit_return_stmt(*inner),   // return_stmt
        visit_if_stmt(*inner),       // if_stmt
        visit_switch_stmt(*inner),   // switch_stmt
        SIR_VISIT_IMPOSSIBLE,        // try_stmt
        SIR_VISIT_IMPOSSIBLE,        // while_stmt
        SIR_VISIT_IMPOSSIBLE,        // for_stmt
        visit_loop_stmt(*inner),     // loop_stmt
        SIR_VISIT_IGNORE,            // continue_stmt
        SIR_VISIT_IGNORE,            // break_stmt
        SIR_VISIT_IGNORE,            // meta_if_stmt
        visit_meta_for_stmt(*inner), // meta_for_stmt
        SIR_VISIT_IGNORE,            // expanded_meta_stmt
        visit_expr(*inner),          // expr_stmt
        visit_block(*inner),         // block_stmt
        SIR_VISIT_IMPOSSIBLE         // error
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

void SpecializationCollector::visit_meta_for_stmt(const sir::MetaForStmt &meta_for_stmt) {
    std::vector<SpecializationCollector::Entry> &entries = specializations.meta_for_entries[&meta_for_stmt];
    sir::GenericParam *param = const_cast<sir::GenericParam *>(meta_for_stmt.generic_param);

    sir::Expr sequence_type = meta_for_stmt.range.get_type();

    if (!entry_stack.empty()) {
        SpecializationCollector::Entry &entry = entry_stack.back();
        sequence_type = sir::Specializer{arena, entry.params, entry.args}.specialize_expr(sequence_type);
    }

    std::span<sir::Expr> sir_types;

    if (auto static_array_type = sequence_type.match<sir::StaticArrayType>()) {
        sir_types = arena.create_array<sir::Expr>({static_array_type->base_type});
    } else if (auto tuple_type = sequence_type.match<sir::TupleExpr>()) {
        sir_types = tuple_type->exprs;
    } else if (auto meta_field_expr = meta_for_stmt.range.match<sir::MetaFieldExpr>()) {
        sir::Expr base_type = meta_field_expr->base.as<sir::MetaAccess>().expr.get_type();

        if (auto reference_type = base_type.match<sir::ReferenceType>()) {
            base_type = reference_type->base_type;
        }

        if (!entry_stack.empty()) {
            SpecializationCollector::Entry &entry = entry_stack.back();
            base_type = sir::Specializer{arena, entry.params, entry.args}.specialize_expr(base_type);
        }

        if (auto struct_def = base_type.match_symbol<sir::StructDef>()) {
            sir_types = arena.allocate_array<sir::Expr>(struct_def->fields.size());

            for (unsigned i = 0; i < struct_def->fields.size(); i++) {
                sir_types[i] = struct_def->fields[i]->type;
            }
        }
    }

    for (sir::Expr sir_type : sir_types) {
        bool found = false;

        for (SpecializationCollector::Entry &entry : entries) {
            if (entry.args[0] == sir_type) {
                found = true;
                break;
            }
        }

        if (found) {
            continue;
        }

        SpecializationCollector::Entry specialization{
            .symbol = nullptr,
            .params = arena.create_array({param}),
            .args{sir_type},
            .resources{},
        };

        entry_stack.push_back(specialization);
        visit_block(*std::get<sir::Block *>(meta_for_stmt.block));
        entries.push_back(std::move(entry_stack.back()));
        entry_stack.pop_back();
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
        SIR_VISIT_IGNORE,                // closure_type (TODO)
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
        SIR_VISIT_IGNORE,                // type_check_expr (TODO)
        visit_placeholder_expr(*inner),  // placeholder_expr
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
    visit_expr(try_expr.type);
    visit_expr(try_expr.value);

    sir::Concrete<sir::StructDef> return_struct_def = try_expr.return_type.as_concrete<sir::StructDef>();
    sir::Concrete<sir::StructDef> struct_def = try_expr.value.get_type().as_concrete<sir::StructDef>();

    sir::SymbolTable &return_struct_symbol_table = *return_struct_def.def->block.symbol_table;
    sir::SymbolTable &symbol_table = *struct_def.def->block.symbol_table;

    sir::Symbol unwrap_func = symbol_table.look_up_local("unwrap");
    sir::Symbol unwrap_error_func = symbol_table.look_up_local("unwrap_error");
    sir::Symbol error_init_func = return_struct_symbol_table.look_up_local("new_failure");

    visit_concrete(unwrap_func, struct_def.generic_args);
    visit_concrete(unwrap_error_func, struct_def.generic_args);
    visit_concrete(error_init_func, return_struct_def.generic_args);
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
    visit_concrete(specialize_expr.symbol, specialize_expr.args);
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
    visit_resource(*deinit_expr.resource, true); // FIXME: Is this always top-level?
}

void SpecializationCollector::visit_placeholder_expr(const sir::PlaceholderExpr &placeholder_expr) {
    if (auto generic_method = std::get_if<sir::PlaceholderExpr::GenericMethod>(&placeholder_expr.kind)) {
        if (!entry_stack.empty()) {
            SpecializationCollector::Entry &entry = entry_stack.back();
            sir::Expr generic_arg = entry.resolve_param(*generic_method->param);

            if (auto concrete_struct = generic_arg.match_concrete<sir::StructDef>()) {
                std::string_view method_name = generic_method->decl->ident.value;
                sir::Symbol func_def = concrete_struct->def->block.symbol_table->look_up(method_name);

                // TODO: The method should always be defined?
                if (func_def) {
                    visit_concrete(func_def, concrete_struct->generic_args);
                }
            }
        }
    } else if (auto binary_expr = std::get_if<sir::PlaceholderExpr::BinaryExpr>(&placeholder_expr.kind)) {
        // TODO: Specialized operator overloads

        visit_expr(binary_expr->lhs);
        visit_expr(binary_expr->rhs);
    } else {
        ASSERT_UNREACHABLE;
    }
}

void SpecializationCollector::visit_concrete(sir::Symbol symbol, std::span<sir::Expr> args) {
    std::vector<sir::Expr> args_full(args.begin(), args.end());

    if (!entry_stack.empty()) {
        SpecializationCollector::Entry &entry = entry_stack.back();

        sir::Specializer specializer{arena, entry.params, entry.args};
        std::span<sir::Expr> specialized_args = specializer.specialize_expr_list(args_full);
        args_full.assign(specialized_args.begin(), specialized_args.end());
    }

    for (Entry &entry : entry_stack) {
        if (entry.symbol == symbol && entry.args == args_full) {
            return;
        }
    }

    std::vector<Entry> &entries = specializations.symbol_entries[symbol];

    for (Entry &entry : entries) {
        if (entry.args == args_full) {
            return;
        }
    }

    std::span<sir::GenericParam *> generic_params;

    if (auto func_def = symbol.match<sir::FuncDef>()) {
        generic_params = func_def->generic_params;
    } else if (auto struct_def = symbol.match<sir::StructDef>()) {
        generic_params = struct_def->generic_params;
    } else if (auto proto_def = symbol.match<sir::ProtoDef>()) {
        generic_params = proto_def->generic_params;
    } else {
        ASSERT_UNREACHABLE;
    }

    entry_stack.push_back(
        Entry{
            .symbol = symbol,
            .params = generic_params,
            .args = args_full,
        }
    );

    if (auto func_def = symbol.match<sir::FuncDef>()) {
        visit_func_def(*func_def, true);
    } else if (auto struct_def = symbol.match<sir::StructDef>()) {
        visit_struct_def(*struct_def, true);
    } else {
        ASSERT_UNREACHABLE;
    }

    entries.push_back(std::move(entry_stack.back()));
    entry_stack.pop_back();
}

void SpecializationCollector::visit_resource(const sir::Resource &resource, bool top_level) {
    std::optional<sir::Resource> specialization;

    if (!entry_stack.empty()) {
        sir::ResourceGenerator resource_generator{arena, resource.ownership, entry_stack.back()};
        specialization = resource_generator.create_resource(resource.type);
    }

    sir::Resource &final_resource = const_cast<sir::Resource &>(specialization ? *specialization : resource);

    if (auto concrete_struct = final_resource.type.match_specialization<sir::StructDef>()) {
        sir::SymbolTable &symbol_table = *concrete_struct->def->block.symbol_table;
        sir::Symbol deinit_symbol = symbol_table.look_up_local(sir::MagicMethods::DEINIT);

        if (deinit_symbol) {
            visit_concrete(deinit_symbol, concrete_struct->generic_args);
        }
    }

    for (const sir::Resource &sub_resource : final_resource.sub_resources) {
        visit_resource(sub_resource, false);
    }

    if (specialization && top_level) {
        entry_stack.back().resources.emplace(&resource, std::move(*specialization));
    }
}

} // namespace banjo::lang
