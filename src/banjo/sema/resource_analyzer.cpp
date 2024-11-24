#include "resource_analyzer.hpp"

#include "banjo/sir/magic_methods.hpp"
#include "banjo/sir/sir.hpp"
#include "banjo/sir/sir_visitor.hpp"

namespace banjo {

namespace lang {

namespace sema {

ResourceAnalyzer::ResourceAnalyzer(SemanticAnalyzer &analyzer) : DeclVisitor(analyzer) {}

Result ResourceAnalyzer::analyze_func_def(sir::FuncDef &func_def) {
    resources_by_symbols.clear();
    super_resources.clear();

    analyze_block(func_def.block);
    return Result::SUCCESS;
}

ResourceAnalyzer::Scope ResourceAnalyzer::analyze_block(sir::Block &block) {
    // TODO: There are performance issues here when analyzing large numbers of resources.
    // One example is `convert.enum_to_repr` with enums that have lots of variants.

    scopes.push_back({
        .block = &block,
        .move_states{},
    });

    for (const auto &[name, symbol] : block.symbol_table->symbols) {
        sir::Expr type;

        if (auto local = symbol.match<sir::Local>()) {
            // TODO: unmanaged attribute

            type = local->type;
        } else if (auto param = symbol.match<sir::Param>()) {
            if (param->attrs && param->attrs->unmanaged) {
                continue;
            }

            type = param->type;
        } else {
            continue;
        }

        if (std::optional<sir::Resource> resource = create_resource(type)) {
            block.resources.push_back({symbol, *resource});
        }
    }

    for (auto &[symbol, resource] : block.resources) {
        insert_move_states(&resource);
        resources_by_symbols.insert({symbol, &resource});
    }

    for (sir::Stmt &stmt : block.stmts) {
        SIR_VISIT_STMT(
            stmt,
            SIR_VISIT_IGNORE,                 // empty
            analyze_var_stmt(*inner),         // var_stmt
            analyze_assign_stmt(*inner),      // assign_stmt
            analyze_comp_assign_stmt(*inner), // comp_assign_stmt
            analyze_return_stmt(*inner),      // return_stmt
            analyze_if_stmt(*inner),          // if_stmt
            SIR_VISIT_IGNORE,                 // switch_stmt
            analyze_try_stmt(*inner),         // try_stmt
            SIR_VISIT_IGNORE,                 // while_stmt
            SIR_VISIT_IGNORE,                 // for_stmt
            analyze_loop_stmt(*inner),        // loop_stmt
            SIR_VISIT_IGNORE,                 // continue_stmt
            SIR_VISIT_IGNORE,                 // break_stmt
            SIR_VISIT_IGNORE,                 // meta_if_stmt
            SIR_VISIT_IGNORE,                 // meta_for_stmt
            SIR_VISIT_IGNORE,                 // expanded_meta_stmt
            analyze_expr(*inner, false),      // expr_stmt
            analyze_block_stmt(*inner),       // block_stmt
            SIR_VISIT_IGNORE                  // error
        );
    }

    Scope child_scope = scopes.back();

    for (auto &[resource, state] : child_scope.move_states) {
        if (state.moved) {
            resource->ownership = state.conditional ? sir::Ownership::MOVED_COND : sir::Ownership::MOVED;
        } else {
            resource->ownership = sir::Ownership::OWNED;
        }
    }

    scopes.pop_back();
    return child_scope;
}

std::optional<sir::Resource> ResourceAnalyzer::create_resource(sir::Expr type) {
    if (auto struct_def = type.match_symbol<sir::StructDef>()) {
        return create_struct_resource(*struct_def, type);
    } else if (auto tuple_type = type.match<sir::TupleExpr>()) {
        return create_tuple_resource(*tuple_type, type);
    } else {
        return {};
    }
}

std::optional<sir::Resource> ResourceAnalyzer::create_struct_resource(sir::StructDef &struct_def, sir::Expr type) {
    std::optional<sir::Resource> resource;

    sir::SymbolTable &symbol_table = *struct_def.block.symbol_table;
    auto iter = symbol_table.symbols.find(sir::MagicMethods::DEINIT);

    if (iter != symbol_table.symbols.end() && iter->second.is<sir::FuncDef>()) {
        resource = sir::Resource{
            .type = type,
            .has_deinit = true,
            .sub_resources = {},
            .ownership = sir::Ownership::OWNED,
        };
    }

    for (unsigned i = 0; i < struct_def.fields.size(); i++) {
        sir::StructField &field = *struct_def.fields[i];
        if (field.attrs && field.attrs->unmanaged) {
            continue;
        }

        std::optional<sir::Resource> sub_resource = create_resource(field.type);
        if (!sub_resource) {
            continue;
        }

        if (!resource) {
            resource = sir::Resource{
                .type = type,
                .has_deinit = false,
                .sub_resources = {},
                .ownership = sir::Ownership::OWNED,
            };
        }

        sub_resource->field_index = i;
        resource->sub_resources.push_back(*sub_resource);
    }

    return resource;
}

std::optional<sir::Resource> ResourceAnalyzer::create_tuple_resource(sir::TupleExpr &tuple_type, sir::Expr type) {
    std::optional<sir::Resource> resource;

    for (unsigned i = 0; i < tuple_type.exprs.size(); i++) {
        std::optional<sir::Resource> sub_resource = create_resource(tuple_type.exprs[i]);
        if (!sub_resource) {
            continue;
        }

        if (!resource) {
            resource = sir::Resource{
                .type = type,
                .has_deinit = false,
                .sub_resources = {},
                .ownership = sir::Ownership::OWNED,
            };
        }

        sub_resource->field_index = i;
        resource->sub_resources.push_back(*sub_resource);
    }

    return resource;
}

void ResourceAnalyzer::insert_move_states(sir::Resource *resource) {
    MoveState state{
        .moved = false,
        .conditional = false,
        .move_expr = nullptr,
    };

    scopes.back().move_states.insert({resource, state});

    for (sir::Resource &sub_resource : resource->sub_resources) {
        insert_move_states(&sub_resource);
        super_resources.insert({&sub_resource, resource});
    }
}

void ResourceAnalyzer::analyze_var_stmt(sir::VarStmt &var_stmt) {
    if (var_stmt.value) {
        analyze_expr(var_stmt.value, true);
    }
}

void ResourceAnalyzer::analyze_assign_stmt(sir::AssignStmt &assign_stmt) {
    analyze_expr(assign_stmt.lhs, false);
    analyze_expr(assign_stmt.rhs, true);
}

void ResourceAnalyzer::analyze_comp_assign_stmt(sir::CompAssignStmt &comp_assign_stmt) {
    analyze_expr(comp_assign_stmt.lhs, false);
    analyze_expr(comp_assign_stmt.rhs, true);
}

void ResourceAnalyzer::analyze_return_stmt(sir::ReturnStmt &return_stmt) {
    analyze_expr(return_stmt.value, true);
}

void ResourceAnalyzer::analyze_if_stmt(sir::IfStmt &if_stmt) {
    std::vector<Scope> child_scopes(if_stmt.cond_branches.size());

    for (unsigned i = 0; i < if_stmt.cond_branches.size(); i++) {
        child_scopes[i] = analyze_block(if_stmt.cond_branches[i].block);
    }

    if (if_stmt.else_branch) {
        child_scopes.push_back(analyze_block(if_stmt.else_branch->block));
    }

    for (Scope &child_scope : child_scopes) {
        merge_move_states(scopes.back(), child_scope, true);
    }
}

void ResourceAnalyzer::analyze_try_stmt(sir::TryStmt &try_stmt) {
    std::vector<Scope> child_scopes{
        analyze_block(try_stmt.success_branch.block),
    };

    if (try_stmt.except_branch) {
        child_scopes.push_back(analyze_block(try_stmt.except_branch->block));
    }

    if (try_stmt.else_branch) {
        child_scopes.push_back(analyze_block(try_stmt.else_branch->block));
    }

    for (Scope &child_scope : child_scopes) {
        merge_move_states(scopes.back(), child_scope, true);
    }
}

void ResourceAnalyzer::analyze_loop_stmt(sir::LoopStmt &loop_stmt) {
    std::vector<Scope> child_scopes{
        analyze_block(loop_stmt.block),
    };

    for (Scope &child_scope : child_scopes) {
        merge_move_states(scopes.back(), child_scope, true);
    }
}

void ResourceAnalyzer::analyze_block_stmt(sir::Block &block) {
    Scope child_scope = analyze_block(block);
    merge_move_states(scopes.back(), child_scope, false);
}

Result ResourceAnalyzer::analyze_expr(sir::Expr &expr, bool moving) {
    Context ctx{
        .moving = moving,
        .field_expr_lhs = false,
        .in_resource_with_deinit = false,
        .in_pointer = false,
        .cur_resource = nullptr,
    };

    return analyze_expr(expr, ctx);
}

Result ResourceAnalyzer::analyze_expr(sir::Expr &expr, Context &ctx) {
    Result result = Result::SUCCESS;

    // FIXME: Not all expression types are handled.
    SIR_VISIT_EXPR(
        expr,
        SIR_VISIT_IGNORE,                                // empty
        SIR_VISIT_IGNORE,                                // int_literal
        SIR_VISIT_IGNORE,                                // fp_literal
        SIR_VISIT_IGNORE,                                // bool_literal
        SIR_VISIT_IGNORE,                                // char_literal
        SIR_VISIT_IGNORE,                                // null_literal
        SIR_VISIT_IGNORE,                                // none_literal
        SIR_VISIT_IGNORE,                                // undefined_literal
        result = analyze_array_literal(*inner, ctx),     // array_literal
        SIR_VISIT_IGNORE,                                // string_literal
        result = analyze_struct_literal(*inner, ctx),    // struct_literal
        SIR_VISIT_IGNORE,                                // union_case_literal
        SIR_VISIT_IGNORE,                                // map_literal
        SIR_VISIT_IGNORE,                                // closure_literal
        result = analyze_symbol_expr(*inner, expr, ctx), // symbol_expr
        SIR_VISIT_IGNORE,                                // binary_expr
        result = analyze_unary_expr(*inner, expr, ctx),  // unary_expr
        SIR_VISIT_IGNORE,                                // cast_expr
        SIR_VISIT_IGNORE,                                // index_expr
        result = analyze_call_expr(*inner, expr, ctx),   // call_expr
        result = analyze_field_expr(*inner, expr, ctx),  // field_expr
        SIR_VISIT_IGNORE,                                // range_expr
        result = analyze_tuple_expr(*inner),             // tuple_expr
        SIR_VISIT_IGNORE,                                // coercion_expr
        SIR_VISIT_IGNORE,                                // primitive_type
        SIR_VISIT_IGNORE,                                // pointer_type
        SIR_VISIT_IGNORE,                                // static_array_type
        SIR_VISIT_IGNORE,                                // func_type
        SIR_VISIT_IGNORE,                                // optional_type
        SIR_VISIT_IGNORE,                                // result_type
        SIR_VISIT_IGNORE,                                // array_type
        SIR_VISIT_IGNORE,                                // map_type
        SIR_VISIT_IGNORE,                                // closure_type
        SIR_VISIT_IGNORE,                                // ident_expr
        SIR_VISIT_IGNORE,                                // star_expr
        SIR_VISIT_IGNORE,                                // bracket_expr
        SIR_VISIT_IGNORE,                                // dot_expr
        SIR_VISIT_IGNORE,                                // pseudo_tpe
        SIR_VISIT_IGNORE,                                // meta_access
        SIR_VISIT_IGNORE,                                // meta_field_expr
        SIR_VISIT_IGNORE,                                // meta_call_expr
        SIR_VISIT_IGNORE,                                // move_expr
        result = analyze_deinit_expr(*inner, expr),      // deinit_expr
        SIR_VISIT_IGNORE                                 // completion_token
    );

    if (ctx.cur_resource && ctx.cur_resource->has_deinit) {
        ctx.in_resource_with_deinit = true;
    }

    // FIXME: This doesn't only apply to call expressions!
    if (!ctx.moving && expr.is<sir::CallExpr>()) {
        sir::Expr type = expr.get_type();

        if (auto resource = create_resource(type)) {
            sir::Resource *temporary_resource = analyzer.cur_sir_mod->create_resource(*resource);

            expr = analyzer.create_expr(sir::DeinitExpr{
                .ast_node = expr.get_ast_node(),
                .type = type,
                .value = expr,
                .resource = temporary_resource,
            });

            ctx.cur_resource = temporary_resource;
            insert_move_states(temporary_resource);
        }
    }

    return result;
}

Result ResourceAnalyzer::analyze_array_literal(sir::ArrayLiteral &array_literal, Context &ctx) {
    Result result = Result::SUCCESS;
    Result partial_result;

    for (sir::Expr &value : array_literal.values) {
        partial_result = analyze_expr(value, true);

        if (partial_result != Result::SUCCESS) {
            result = Result::ERROR;
        }
    }

    return result;
}

Result ResourceAnalyzer::analyze_struct_literal(sir::StructLiteral &struct_literal, Context &ctx) {
    Result result = Result::SUCCESS;
    Result partial_result;

    for (sir::StructLiteralEntry &entry : struct_literal.entries) {
        partial_result = analyze_expr(entry.value, true);

        if (partial_result != Result::SUCCESS) {
            result = Result::ERROR;
        }
    }

    return result;
}

Result ResourceAnalyzer::analyze_unary_expr(sir::UnaryExpr &unary_expr, sir::Expr &out_expr, Context &ctx) {
    if (unary_expr.op == sir::UnaryOp::DEREF) {
        Result result;

        if (ctx.moving && is_resource(unary_expr.type)) {
            analyzer.report_generator.report_err_move_out_pointer(&unary_expr);
            result = Result::ERROR;
        } else {
            result = analyze_expr(unary_expr.value, ctx);
        }

        ctx.in_pointer = true;
        return result;
    } else if (unary_expr.op == sir::UnaryOp::REF) {
        return analyze_expr(unary_expr.value, false);
    } else {
        return analyze_expr(unary_expr.value, ctx);
    }
}

Result ResourceAnalyzer::analyze_symbol_expr(sir::SymbolExpr &symbol_expr, sir::Expr &out_expr, Context &ctx) {
    auto resource_iter = resources_by_symbols.find(symbol_expr.symbol);
    if (resource_iter == resources_by_symbols.end()) {
        return Result::SUCCESS;
    }

    sir::Resource *resource = resource_iter->second;
    return analyze_resource_use(resource, out_expr, ctx);
}

Result ResourceAnalyzer::analyze_call_expr(sir::CallExpr &call_expr, sir::Expr &out_expr, Context &ctx) {
    Result result = Result::SUCCESS;
    Result partial_result;

    for (sir::Expr &arg : call_expr.args) {
        partial_result = analyze_expr(arg, true);

        if (partial_result != Result::SUCCESS) {
            result = Result::ERROR;
        }
    }

    return result;
}

Result ResourceAnalyzer::analyze_field_expr(sir::FieldExpr &field_expr, sir::Expr &out_expr, Context &ctx) {
    Context lhs_ctx{
        .moving = false,
        .field_expr_lhs = true,
        .in_resource_with_deinit = false,
        .in_pointer = false,
        .cur_resource = ctx.cur_resource,
    };

    Result lhs_result = analyze_expr(field_expr.base, lhs_ctx);

    ctx.in_resource_with_deinit = lhs_ctx.in_resource_with_deinit;
    ctx.in_pointer = lhs_ctx.in_pointer;
    ctx.cur_resource = lhs_ctx.cur_resource;

    if (lhs_result != Result::SUCCESS) {
        return Result::ERROR;
    }

    if (!ctx.cur_resource) {
        if (ctx.moving && ctx.in_pointer && is_resource(field_expr.type)) {
            analyzer.report_generator.report_err_move_out_pointer(out_expr);
            return Result::ERROR;
        }

        return Result::SUCCESS;
    }

    for (sir::Resource &sub_resource : ctx.cur_resource->sub_resources) {
        if (sub_resource.field_index == field_expr.field_index) {
            if (ctx.moving && ctx.in_resource_with_deinit) {
                analyzer.report_generator.report_err_move_out_deinit(&field_expr);
                return Result::ERROR;
            }

            analyze_resource_use(&sub_resource, out_expr, ctx);
            return Result::SUCCESS;
        }
    }

    ctx.cur_resource = nullptr;
    return Result::SUCCESS;
}

Result ResourceAnalyzer::analyze_tuple_expr(sir::TupleExpr &tuple_expr) {
    Result result = Result::SUCCESS;
    Result partial_result;

    for (sir::Expr &expr : tuple_expr.exprs) {
        partial_result = analyze_expr(expr, true);

        if (partial_result != Result::SUCCESS) {
            result = Result::ERROR;
        }
    }

    return result;
}

Result ResourceAnalyzer::analyze_deinit_expr(sir::DeinitExpr &deinit_expr, sir::Expr &out_expr) {
    if (!deinit_expr.resource) {
        if (std::optional<sir::Resource> resource = create_resource(deinit_expr.type)) {
            deinit_expr.resource = analyzer.cur_sir_mod->create_resource(*resource);
        } else {
            out_expr = deinit_expr.value;
        }
    }

    return Result::SUCCESS;
}

Result ResourceAnalyzer::analyze_resource_use(sir::Resource *resource, sir::Expr &inout_expr, Context &ctx) {
    ctx.cur_resource = resource;

    MoveState *state = find_move_state(resource);
    if (!state) {
        return Result::SUCCESS;
    }

    if (state->moved && !(state->partial && ctx.field_expr_lhs)) {
        analyzer.report_generator
            .report_err_use_after_move(inout_expr, state->move_expr, state->partial, state->conditional);
        return Result::ERROR;
    }

    if (ctx.moving) {
        scopes.back().move_states[resource] = MoveState{
            .moved = true,
            .conditional = false,
            .partial = false,
            .move_expr = inout_expr,
        };

        auto super_resource_iter = super_resources.find(resource);

        while (super_resource_iter != super_resources.end()) {
            sir::Resource *super_resource = super_resource_iter->second;

            scopes.back().move_states[super_resource] = MoveState{
                .moved = true,
                .conditional = false,
                .partial = true,
                .move_expr = inout_expr,
            };

            super_resource_iter = super_resources.find(super_resource);
        }

        move_sub_resources(resource, inout_expr);

        inout_expr = analyzer.create_expr(sir::MoveExpr{
            .ast_node = inout_expr.get_ast_node(),
            .type = inout_expr.get_type(),
            .value = inout_expr,
            .resource = resource,
        });
    }

    return Result::SUCCESS;
}

ResourceAnalyzer::MoveState *ResourceAnalyzer::find_move_state(sir::Resource *resource) {
    for (auto scope_iter = scopes.rbegin(); scope_iter != scopes.rend(); scope_iter++) {
        Scope &scope = *scope_iter;

        auto state_iter = scope.move_states.find(resource);
        if (state_iter != scope.move_states.end()) {
            return &state_iter->second;
        }
    }

    return nullptr;
}

void ResourceAnalyzer::move_sub_resources(sir::Resource *resource, sir::Expr move_expr) {
    for (sir::Resource &sub_resource : resource->sub_resources) {
        scopes.back().move_states[&sub_resource] = MoveState{
            .moved = true,
            .conditional = false,
            .partial = true,
            .move_expr = move_expr,
        };

        move_sub_resources(&sub_resource, move_expr);
    }
}

unsigned ResourceAnalyzer::get_scope_depth() {
    return static_cast<unsigned>(scopes.size() - 1);
}

bool ResourceAnalyzer::is_resource(const sir::Expr &type) {
    if (auto struct_def = type.match_symbol<sir::StructDef>()) {
        sir::SymbolTable &symbol_table = *struct_def->block.symbol_table;
        auto iter = symbol_table.symbols.find(sir::MagicMethods::DEINIT);
        if (iter != symbol_table.symbols.end() && iter->second.is<sir::FuncDef>()) {
            return true;
        }

        for (const sir::StructField *field : struct_def->fields) {
            if (is_resource(field->type)) {
                return true;
            }
        }

        return false;
    } else if (auto tuple_type = type.match<sir::TupleExpr>()) {
        for (const sir::Expr &type : tuple_type->exprs) {
            if (is_resource(type)) {
                return true;
            }
        }

        return false;
    } else {
        return false;
    }
}

void ResourceAnalyzer::merge_move_states(Scope &parent_scope, Scope &child_scope, bool conditional) {
    for (auto &[symbol, state] : child_scope.move_states) {
        if (!state.moved) {
            continue;
        }

        auto iter = parent_scope.move_states.find(symbol);

        if (iter == parent_scope.move_states.end()) {
            // If the resource does not exist in the parent scope, set it to moved.

            state.conditional = conditional;
            parent_scope.move_states.insert({symbol, state});
            continue;
        }

        MoveState &parent_state = iter->second;

        if (!parent_state.moved) {
            // If the resource is not moved in the parent scope, set it to moved.

            parent_state.moved = true;
            parent_state.conditional = conditional;
            parent_state.partial = state.partial;
            parent_state.move_expr = state.move_expr;
        } else if (!conditional && parent_state.conditional) {
            // If the resource is moved only conditionally in the parent scope, but
            // unconditionally in the child scope, set it to unconditionally moved
            // in the parent scope and update the expression that moved it.

            parent_state.conditional = false;
            parent_state.move_expr = state.move_expr;
        }
    }
}

} // namespace sema

} // namespace lang

} // namespace banjo
