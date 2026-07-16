#include "block_ssa_generator.hpp"

#include "banjo/sir/magic_methods.hpp"
#include "banjo/sir/sir.hpp"
#include "banjo/sir/sir_visitor.hpp"
#include "banjo/ssa/comparison.hpp"
#include "banjo/ssa/virtual_register.hpp"
#include "banjo/ssa_gen/expr_ssa_generator.hpp"
#include "banjo/ssa_gen/specialization_collector.hpp"
#include "banjo/ssa_gen/ssa_generator_context.hpp"
#include "banjo/ssa_gen/storage_hints.hpp"
#include "banjo/ssa_gen/stored_value.hpp"
#include "banjo/ssa_gen/type_ssa_generator.hpp"
#include "banjo/utils/macros.hpp"

#include <ranges>
#include <utility>

namespace banjo {

namespace lang {

BlockSSAGenerator::BlockSSAGenerator(SSAGeneratorContext &ctx) : ctx(ctx) {}

void BlockSSAGenerator::generate_block(const sir::Block &block) {
    generate_block_allocas(block);
    generate_block_body(block);
}

void BlockSSAGenerator::generate_block_allocas(const sir::Block &block) {
    for (const auto &[name, symbol] : block.symbol_table->symbols) {
        if (auto local = symbol.match<sir::Local>()) {
            ssa::Type ssa_type = TypeSSAGenerator(ctx).generate(local->type);
            ssa::VirtualRegister reg = ctx.append_alloca(ssa_type);
            ctx.ssa_local_regs[local] = reg;
        }
    }

    for (const auto &[symbol, resource] : block.resources) {
        generate_resource_flags(ctx.resolve_resource(resource));
    }
}

void BlockSSAGenerator::generate_resource_flags(const sir::Resource &resource) {
    if (resource.ownership == sir::Ownership::MOVED_COND) {
        generate_resource_flag_slot(resource, DEINIT_FLAG_TRUE);
    } else if (resource.ownership == sir::Ownership::INIT_COND) {
        generate_resource_flag_slot(resource, DEINIT_FLAG_FALSE);
    }

    for (const sir::Resource &sub_resource : resource.sub_resources) {
        generate_resource_flags(sub_resource);
    }
}

void BlockSSAGenerator::generate_resource_flag_slot(const sir::Resource &resource, ssa::Value initial_value) {
    ssa::VirtualRegister flag_slot = ctx.append_alloca(ssa::Primitive::U8);
    ctx.append_store(std::move(initial_value), flag_slot);
    ctx.get_func_context().resource_deinit_flags.emplace(&resource, flag_slot);
}

void BlockSSAGenerator::generate_block_body(const sir::Block &block) {
    ctx.get_func_context().sir_scopes.push_back(&block);

    for (sir::Stmt sir_stmt : block.stmts) {
        generate_stmt(sir_stmt);

        if (ctx.get_ssa_block()->is_branching()) {
            ctx.get_func_context().sir_scopes.pop_back();
            return;
        }
    }

    ctx.get_func_context().sir_scopes.pop_back();

    generate_block_deinit(block);
}

void BlockSSAGenerator::generate_block_deinit(const sir::Block &block) {
    for (const auto &[symbol, resource] : std::ranges::reverse_view(block.resources)) {
        generate_deinit(resource, symbol);
    }
}

void BlockSSAGenerator::generate_stmt(sir::Stmt sir_stmt) {
    SIR_VISIT_STMT(
        sir_stmt,
        SIR_VISIT_IMPOSSIBLE,                                           // empty
        generate_var_stmt(*inner),                                      // var_stmt
        generate_assign_stmt(*inner),                                   // assign_stmt
        SIR_VISIT_IMPOSSIBLE,                                           // comp_assign_stmt
        generate_return_stmt(*inner),                                   // return_stmt
        generate_if_stmt(*inner),                                       // if_stmt
        generate_switch_stmt(*inner),                                   // switch_stmt
        SIR_VISIT_IMPOSSIBLE,                                           // try_stmt
        SIR_VISIT_IMPOSSIBLE,                                           // while_stmt
        SIR_VISIT_IMPOSSIBLE,                                           // for_stmt
        generate_loop_stmt(*inner),                                     // loop_stmt
        generate_continue_stmt(*inner),                                 // continue_stmt
        generate_break_stmt(*inner),                                    // break_stmt
        SIR_VISIT_IGNORE,                                               // meta_if_stmt
        generate_meta_for_stmt(*inner),                                 // meta_for_stmt
        SIR_VISIT_IGNORE,                                               // expanded_meta_stmt
        ExprSSAGenerator(ctx).generate(*inner, StorageHints::unused()), // expr_stmt
        generate_block(*inner),                                         // block_stmt
        return                                                          // error
    );

    if (ctx.get_ssa_block()->is_branching()) {
        return;
    }

    for (DeferredDeinit &deferred_deinit : ctx.get_func_context().cur_deferred_deinits) {
        generate_deinit(*deferred_deinit.resource, deferred_deinit.ssa_ptr);
    }

    ctx.get_func_context().cur_deferred_deinits.clear();
}

void BlockSSAGenerator::generate_var_stmt(const sir::VarStmt &var_stmt) {
    if (var_stmt.value) {
        ssa::VirtualRegister reg = ctx.ssa_local_regs[&var_stmt.local];
        ssa::Value ssa_ptr = ssa::Value::from_register(reg, ssa::Primitive::ADDR);
        ExprSSAGenerator(ctx).generate_into_dst(var_stmt.value, ssa_ptr);
    }
}

void BlockSSAGenerator::generate_assign_stmt(const sir::AssignStmt &assign_stmt) {
    StoredValue dst = ExprSSAGenerator(ctx).generate(assign_stmt.lhs, StorageHints::prefer_reference());
    ExprSSAGenerator(ctx).generate_into_dst(assign_stmt.rhs, dst.get_ptr());
}

void BlockSSAGenerator::generate_return_stmt(const sir::ReturnStmt &return_stmt) {
    SSAGeneratorContext::FuncContext &func_context = ctx.get_func_context();

    if (return_stmt.value) {
        ExprSSAGenerator(ctx).generate_into_dst(return_stmt.value, ctx.get_func_context().ssa_return_slot);
    }

    for (auto iter = func_context.sir_scopes.rbegin(); iter != func_context.sir_scopes.rend(); ++iter) {
        generate_block_deinit(**iter);
    }

    ctx.append_jmp(ctx.get_func_context().ssa_func_exit);
}

void BlockSSAGenerator::generate_if_stmt(const sir::IfStmt &if_stmt) {
    std::vector<unsigned> branches;
    std::optional<unsigned> else_branch;

    if (if_stmt.else_branch) {
        else_branch = if_stmt.cond_branches.size();
    }

    for (unsigned i = 0; i < if_stmt.cond_branches.size(); i++) {
        const sir::IfCondBranch &sir_branch = if_stmt.cond_branches[i];

        if (auto type_guard_expr = sir_branch.condition.match<sir::TypeGuardExpr>()) {
            sir::Expr arg = ctx.get_generic_arg(*type_guard_expr->generic_param);

            if (ctx.is_type_guard_satisfied(*type_guard_expr, arg)) {
                else_branch = i;
                break;
            }
        } else {
            branches.push_back(i);
        }
    }

    ssa::BasicBlockIter ssa_end_block;

    if (!branches.empty()) {
        ssa_end_block = ctx.create_block();
    }

    for (unsigned i = 0; i < branches.size(); i++) {
        const sir::IfCondBranch &sir_branch = if_stmt.cond_branches[branches[i]];
        bool is_final_branch = i == branches.size() - 1 && !else_branch;

        ssa::BasicBlockIter ssa_next_block = is_final_branch ? nullptr : ctx.create_block();
        ssa::BasicBlockIter ssa_target_if_true = ctx.create_block();
        ssa::BasicBlockIter ssa_target_if_false = is_final_branch ? ssa_end_block : ssa_next_block;

        ExprSSAGenerator(ctx).generate_branch(sir_branch.condition, {ssa_target_if_true, ssa_target_if_false});

        ctx.append_block(ssa_target_if_true);
        generate_block(*sir_branch.block);
        ctx.append_jmp(ssa_end_block);

        if (!is_final_branch) {
            ctx.append_block(ssa_next_block);
        }
    }

    if (else_branch) {
        if (*else_branch == if_stmt.cond_branches.size()) {
            generate_block(*if_stmt.else_branch->block);
        } else {
            generate_block(*if_stmt.cond_branches[*else_branch].block);
        }

        if (!branches.empty()) {
            ctx.append_jmp(ssa_end_block);
        }
    }

    if (!branches.empty()) {
        ctx.append_block(ssa_end_block);
    }
}

void BlockSSAGenerator::generate_switch_stmt(const sir::SwitchStmt &switch_stmt) {
    const sir::UnionDef &sir_union_def = switch_stmt.value.get_type().as_symbol<sir::UnionDef>();

    ssa::BasicBlockIter ssa_end_block = ctx.create_block();

    StoredValue ssa_value = ExprSSAGenerator(ctx).generate(switch_stmt.value);
    ssa::VirtualRegister ssa_tag_ptr_reg = ctx.append_memberptr(ssa_value.value_type, ssa_value.get_ptr(), 0);
    ssa::Value ssa_tag = ctx.append_load(ssa::Primitive::U32, ssa_tag_ptr_reg);

    for (unsigned i = 0; i < switch_stmt.case_branches.size(); i++) {
        const sir::SwitchCaseBranch &sir_branch = switch_stmt.case_branches[i];
        const sir::UnionCase &sir_union_case = sir_branch.local.type.as_symbol<sir::UnionCase>();
        unsigned tag = sir_union_def.get_index(sir_union_case);
        bool is_final_branch = i == switch_stmt.case_branches.size() - 1;

        ssa::BasicBlockIter ssa_next_block = is_final_branch ? nullptr : ctx.create_block();
        ssa::BasicBlockIter ssa_target_if_true = ctx.create_block();
        ssa::BasicBlockIter ssa_target_if_false = is_final_branch ? ssa_end_block : ssa_next_block;

        ssa::Value ssa_tag_value = ssa::Value::from_int_immediate(tag, ssa::Primitive::U32);
        ctx.append_cjmp(ssa_tag, ssa::Comparison::EQ, ssa_tag_value, ssa_target_if_true, ssa_target_if_false);
        ctx.append_block(ssa_target_if_true);

        generate_block_allocas(*sir_branch.block);

        ssa::Type ssa_case_type = TypeSSAGenerator(ctx).generate(sir_branch.local.type);
        ssa::VirtualRegister ssa_data_ptr_reg = ctx.append_memberptr(ssa_value.value_type, ssa_value.get_ptr(), 1);
        StoredValue ssa_data_ptr = StoredValue::create_reference(ssa_data_ptr_reg, ssa_case_type);
        ssa::VirtualRegister ssa_local_reg = ctx.ssa_local_regs[&sir_branch.local];
        ssa_data_ptr.copy_to(ssa_local_reg, ctx);

        generate_block_body(*sir_branch.block);
        ctx.append_jmp(ssa_end_block);

        if (!is_final_branch) {
            ctx.append_block(ssa_next_block);
        }
    }

    ctx.append_block(ssa_end_block);
}

void BlockSSAGenerator::generate_loop_stmt(const sir::LoopStmt &loop_stmt) {
    ssa::BasicBlockIter ssa_cond_block = ctx.create_block();
    ssa::BasicBlockIter ssa_body_entry_block = ctx.create_block();
    ssa::BasicBlockIter ssa_latch_block = loop_stmt.latch ? ctx.create_block() : nullptr;
    ssa::BasicBlockIter ssa_end_block = ctx.create_block();

    ctx.append_jmp(ssa_cond_block);
    ctx.append_block(ssa_cond_block);

    ExprSSAGenerator(ctx).generate_branch(loop_stmt.condition, {ssa_body_entry_block, ssa_end_block});

    ctx.push_loop_context({
        .sir_block = loop_stmt.block,
        .ssa_continue_target = loop_stmt.latch ? ssa_latch_block : ssa_cond_block,
        .ssa_break_target = ssa_end_block,
    });

    ctx.append_block(ssa_body_entry_block);
    generate_block(*loop_stmt.block);

    ctx.pop_loop_context();

    if (loop_stmt.latch) {
        ctx.append_jmp(ssa_latch_block);
        ctx.append_block(ssa_latch_block);
        generate_block(*loop_stmt.latch);
    }

    ctx.append_jmp(ssa_cond_block);
    ctx.append_block(ssa_end_block);
}

void BlockSSAGenerator::generate_continue_stmt(const sir::ContinueStmt & /*continue_stmt*/) {
    generate_loop_jump_deinit();
    ctx.append_jmp(ctx.get_loop_context().ssa_continue_target);
}

void BlockSSAGenerator::generate_break_stmt(const sir::BreakStmt & /*break_stmt*/) {
    generate_loop_jump_deinit();
    ctx.append_jmp(ctx.get_loop_context().ssa_break_target);
}

void BlockSSAGenerator::generate_meta_for_stmt(const sir::MetaForStmt &meta_for_stmt) {
    utils::Arena arena;

    sir::Expr sequence_type = ctx.resolve_if_generic(meta_for_stmt.range.get_type());
    std::span<sir::Expr> values;
    std::span<sir::Expr> generic_args;

    StoredValue ssa_base;

    if (auto static_array_type = sequence_type.match<sir::StaticArrayType>()) {
        sir::Expr base_type = ctx.resolve_if_generic(static_array_type->base_type);

        // HACK: This is horrible
        unsigned length =
            ExprSSAGenerator{ctx}.generate(static_array_type->length).get_value().get_int_immediate().to_unsigned();

        values = arena.allocate_array<sir::Expr>(length);
        generic_args = arena.allocate_array<sir::Expr>(length);

        for (unsigned i = 0; i < values.size(); i++) {
            values[i] = arena.create(
                sir::IndexExpr{
                    .ast_node = nullptr,
                    .type = base_type,
                    .base = meta_for_stmt.range,
                    .index = arena.create(
                        sir::IntLiteral{
                            .ast_node = nullptr,
                            .type = arena.create(
                                sir::PrimitiveType{
                                    .ast_node = nullptr,
                                    .primitive = sir::Primitive::USIZE,
                                }
                            ),
                            .value = i,
                        }
                    ),
                }
            );

            generic_args[i] = base_type;
        }

        ssa_base = ExprSSAGenerator{ctx}.generate_as_reference(meta_for_stmt.range);
    } else if (auto tuple_type = sequence_type.match<sir::TupleExpr>()) {
        values = arena.allocate_array<sir::Expr>(tuple_type->exprs.size());
        generic_args = arena.allocate_array<sir::Expr>(tuple_type->exprs.size());

        for (unsigned i = 0; i < values.size(); i++) {
            values[i] = arena.create(
                sir::FieldExpr{
                    .ast_node = nullptr,
                    .type = tuple_type->exprs[i],
                    .base = meta_for_stmt.range,
                    .field_index = i,
                }
            );

            generic_args[i] = tuple_type->exprs[i];
        }

        ssa_base = ExprSSAGenerator{ctx}.generate_as_reference(meta_for_stmt.range);
    } else if (auto meta_field_expr = meta_for_stmt.range.match<sir::MetaFieldExpr>()) {
        sir::Expr base = meta_field_expr->base.as<sir::MetaAccess>().expr;
        sir::Expr base_type = base.get_type();

        if (auto reference_type = base_type.match<sir::ReferenceType>()) {
            base_type = reference_type->base_type;
        }

        base_type = ctx.resolve_if_generic(base_type);

        if (auto struct_def = base_type.match_symbol<sir::StructDef>()) {
            values = arena.allocate_array<sir::Expr>(struct_def->fields.size());
            generic_args = arena.allocate_array<sir::Expr>(struct_def->fields.size());

            sir::Expr string_type = arena.create(
                sir::PointerType{
                    .ast_node = nullptr,
                    .base_type = arena.create(sir::PrimitiveType{.ast_node = nullptr, .primitive = sir::Primitive::U8}),
                }
            );

            for (unsigned i = 0; i < values.size(); i++) {
                sir::Expr string_literal = arena.create(
                    sir::StringLiteral{
                        .ast_node = nullptr,
                        .type = string_type,
                        .value = struct_def->fields[i]->ident.value,
                    }
                );

                sir::Expr field_expr = arena.create(
                    sir::FieldExpr{
                        .ast_node = nullptr,
                        .type = struct_def->fields[i]->type,
                        .base = base,
                        .field_index = i,
                    }
                );

                sir::Expr tuple_type = arena.create(
                    sir::TupleExpr{
                        .ast_node = nullptr,
                        .type = nullptr,
                        .exprs = arena.create_array({string_type, struct_def->fields[i]->type}),
                    }
                );

                values[i] = arena.create(
                    sir::TupleExpr{
                        .ast_node = nullptr,
                        .type = tuple_type,
                        .exprs = arena.create_array({string_literal, field_expr}),
                    }
                );

                generic_args[i] = struct_def->fields[i]->type;
            }

            ssa_base = ExprSSAGenerator{ctx}.generate_as_reference(base);
        }
    }

    std::vector<SpecializationCollector::Entry> &specializations =
        ctx.specializations.meta_for_entries.at(&meta_for_stmt);

    for (unsigned i = 0; i < values.size(); i++) {
        ssa::Type ssa_type = TypeSSAGenerator{ctx}.generate(values[i].get_type());
        ssa::VirtualRegister dst = ctx.append_alloca(ssa_type);
        ExprSSAGenerator{ctx}.generate_as_reference(values[i]).copy_to(dst, ctx);

        ctx.ssa_local_regs[&meta_for_stmt.local] = dst;

        SpecializationCollector::Entry *specialization = nullptr;

        for (SpecializationCollector::Entry &candidate : specializations) {
            if (candidate.args[0] == generic_args[i]) {
                specialization = &candidate;
                break;
            }
        }

        ASSERT(specialization);

        ctx.push_specialization(*specialization);
        generate_block_body(*std::get<sir::Block *>(meta_for_stmt.block));
        ctx.pop_specialization(*specialization);
    }
}

void BlockSSAGenerator::generate_loop_jump_deinit() {
    SSAGeneratorContext::FuncContext &func_context = ctx.get_func_context();
    SSAGeneratorContext::LoopContext &loop_context = ctx.get_loop_context();

    for (auto iter = func_context.sir_scopes.rbegin(); iter != func_context.sir_scopes.rend(); ++iter) {
        generate_block_deinit(**iter);

        if (*iter == loop_context.sir_block) {
            break;
        }
    }
}

void BlockSSAGenerator::generate_deinit(const sir::Resource &resource, sir::Symbol symbol) {
    sir::Expr type = symbol.get_type();

    sir::SymbolExpr ptr{
        .ast_node = nullptr,
        .type = type,
        .symbol = symbol,
    };

    ssa::Value ssa_ptr = ExprSSAGenerator(ctx).generate_as_reference(&ptr).get_ptr();
    generate_deinit(resource, ssa_ptr);
}

void BlockSSAGenerator::generate_deinit(const sir::Resource &resource, ssa::Value ssa_ptr) {
    const sir::Resource &final_resource = ctx.resolve_resource(resource);

    if (final_resource.ownership == sir::Ownership::OWNED) {
        generate_deinit_call(final_resource, std::move(ssa_ptr));
    } else if (
        final_resource.ownership == sir::Ownership::MOVED_COND || final_resource.ownership == sir::Ownership::INIT_COND
    ) {
        ssa::BasicBlockIter deinit_block = ctx.create_block();
        ssa::BasicBlockIter end_block = ctx.create_block();

        ssa::VirtualRegister flag_slot = ctx.get_func_context().resource_deinit_flags.at(&final_resource);
        ssa::Value flag_val = ctx.append_load(ssa::Primitive::U8, flag_slot);
        ctx.append_cjmp(flag_val, ssa::Comparison::NE, DEINIT_FLAG_FALSE, deinit_block, end_block);

        ctx.append_block(deinit_block);
        generate_deinit_call(final_resource, std::move(ssa_ptr));
        ctx.append_jmp(end_block);
        ctx.append_block(end_block);
    }

    ssa::Type ssa_type = TypeSSAGenerator(ctx).generate(final_resource.type);

    for (const sir::Resource &sub_resource : final_resource.sub_resources) {
        ssa::VirtualRegister field_ptr_reg = ctx.append_memberptr(ssa_type, ssa_ptr, sub_resource.field_index);
        ssa::Value field_ptr = ssa::Value::from_register(field_ptr_reg, ssa::Primitive::ADDR);
        generate_deinit(sub_resource, field_ptr);
    }
}

void BlockSSAGenerator::generate_deinit_call(const sir::Resource &resource, ssa::Value ssa_ptr) {
    sir::SymbolTable *symbol_table;
    std::span<sir::Expr> generic_args{static_cast<sir::Expr *>(nullptr), 0};

    if (auto concrete_struct = resource.type.match_concrete<sir::StructDef>()) {
        symbol_table = concrete_struct->def->block.symbol_table;
        generic_args = concrete_struct->generic_args;
    } else if (auto closure_type = resource.type.match<sir::ClosureType>()) {
        symbol_table = closure_type->underlying_struct->block.symbol_table;
        return; // TODO: generics
    } else {
        return;
    }

    sir::Symbol deinit_symbol = symbol_table->look_up_local(sir::MagicMethods::DEINIT);
    if (!deinit_symbol) {
        return;
    }

    ssa::Function *ssa_func;

    if (generic_args.empty()) {
        ssa_func = ctx.ssa_funcs.at(&deinit_symbol.as<sir::FuncDef>());
    } else {
        ssa_func = &ctx.find_ssa_func({&deinit_symbol.as<sir::FuncDef>(), generic_args});
    }

    ssa::Value ssa_callee = ssa::Operand::from_func(ssa_func, ssa::Primitive::ADDR);
    ctx.get_ssa_block()->append({ssa::Opcode::CALL, {ssa_callee, std::move(ssa_ptr)}});
}

} // namespace lang

} // namespace banjo
