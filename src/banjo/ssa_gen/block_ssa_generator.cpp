#include "block_ssa_generator.hpp"

#include "banjo/sir/sir.hpp"
#include "banjo/sir/sir_visitor.hpp"
#include "banjo/ssa/comparison.hpp"
#include "banjo/ssa/virtual_register.hpp"
#include "banjo/ssa_gen/deinit_ssa_generator.hpp"
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

BlockSSAGenerator::BlockSSAGenerator(SSAGeneratorContext &ctx) : ctx{ctx} {}

void BlockSSAGenerator::generate_block(const sir::Block &block) {
    generate_block_allocas(block);
    generate_block_body(block);
}

void BlockSSAGenerator::generate_block_allocas(const sir::Block &block, const sir::Local *excluded /* = nullptr */) {
    for (const auto &[name, symbol] : block.symbol_table->symbols) {
        if (auto local = symbol.match<sir::Local>()) {
            if (local == excluded) {
                continue;
            }

            ssa::Type ssa_type = TypeSSAGenerator{ctx}.generate(local->type);
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
    for (const auto &[symbol, resource] : std::ranges::reverse_view{block.resources}) {
        DeinitSSAGenerator{ctx}.generate_deinit(resource, symbol);
    }
}

void BlockSSAGenerator::generate_stmt(sir::Stmt sir_stmt) {
    SIR_VISIT_STMT(
        sir_stmt,
        SIR_VISIT_IMPOSSIBLE,           // empty
        generate_var_stmt(*inner),      // var_stmt
        generate_assign_stmt(*inner),   // assign_stmt
        SIR_VISIT_IMPOSSIBLE,           // comp_assign_stmt
        generate_return_stmt(*inner),   // return_stmt
        generate_switch_stmt(*inner),   // switch_stmt
        SIR_VISIT_IMPOSSIBLE,           // try_stmt
        SIR_VISIT_IMPOSSIBLE,           // while_stmt
        SIR_VISIT_IMPOSSIBLE,           // for_stmt
        generate_loop_stmt(*inner),     // loop_stmt
        generate_continue_stmt(*inner), // continue_stmt
        generate_break_stmt(*inner),    // break_stmt
        SIR_VISIT_IGNORE,               // meta_if_stmt
        generate_meta_for_stmt(*inner), // meta_for_stmt
        SIR_VISIT_IGNORE,               // expanded_meta_stmt
        generate_expr_stmt(*inner),     // expr_stmt
        generate_block(*inner),         // block_stmt
        return                          // error
    );
}

void BlockSSAGenerator::generate_var_stmt(const sir::VarStmt &var_stmt) {
    if (var_stmt.value) {
        ssa::VirtualRegister reg = ctx.ssa_local_regs.at(&var_stmt.local);
        ssa::Value ssa_ptr = ssa::Value::from_register(reg, ssa::Primitive::ADDR);
        ExprSSAGenerator{ctx}.generate_into_dst(var_stmt.value, ssa_ptr);

        DeinitSSAGenerator{ctx}.generate_deferred_deinits();
    }
}

void BlockSSAGenerator::generate_assign_stmt(const sir::AssignStmt &assign_stmt) {
    StoredValue dst = ExprSSAGenerator{ctx}.generate(assign_stmt.lhs, StorageHints::prefer_reference());
    ExprSSAGenerator{ctx}.generate_into_dst(assign_stmt.rhs, dst.get_ptr());

    DeinitSSAGenerator{ctx}.generate_deferred_deinits();
}

void BlockSSAGenerator::generate_return_stmt(const sir::ReturnStmt &return_stmt) {
    SSAGeneratorContext::FuncContext &func_context = ctx.get_func_context();

    if (return_stmt.value) {
        ExprSSAGenerator{ctx}.generate_into_dst(return_stmt.value, ctx.get_func_context().ssa_return_slot);
    }

    DeinitSSAGenerator{ctx}.generate_deferred_deinits();

    for (auto iter = func_context.sir_scopes.rbegin(); iter != func_context.sir_scopes.rend(); ++iter) {
        generate_block_deinit(**iter);
    }

    ctx.append_jmp(ctx.get_func_context().ssa_func_exit);
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

        ssa::Type ssa_case_type = TypeSSAGenerator{ctx}.generate(sir_branch.local.type);
        ssa::VirtualRegister ssa_data_ptr_reg = ctx.append_memberptr(ssa_value.value_type, ssa_value.get_ptr(), 1);
        StoredValue ssa_data_ptr = StoredValue::create_reference(ssa_data_ptr_reg, ssa_case_type);
        ssa::VirtualRegister ssa_local_reg = ctx.ssa_local_regs.at(&sir_branch.local);
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

    ExprSSAGenerator{ctx}.generate_branch(loop_stmt.condition, {ssa_body_entry_block, ssa_end_block});
    DeinitSSAGenerator{ctx}.generate_deferred_deinits();

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
    ASSERT(ctx.get_func_context().cur_deferred_deinits.empty());

    generate_loop_jump_deinit();
    ctx.append_jmp(ctx.get_loop_context().ssa_continue_target);
}

void BlockSSAGenerator::generate_break_stmt(const sir::BreakStmt & /*break_stmt*/) {
    ASSERT(ctx.get_func_context().cur_deferred_deinits.empty());

    generate_loop_jump_deinit();
    ctx.append_jmp(ctx.get_loop_context().ssa_break_target);
}

void BlockSSAGenerator::generate_meta_for_stmt(const sir::MetaForStmt &meta_for_stmt) {
    // TODO: Deinits

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
            values[i] = arena.create<sir::IndexExpr>({
                .ast_node = nullptr,
                .type = base_type,
                .base = meta_for_stmt.range,
                .index = arena.create<sir::IntLiteral>({
                    .ast_node = nullptr,
                    .type = arena.create<sir::PrimitiveType>({
                        .ast_node = nullptr,
                        .primitive = sir::Primitive::USIZE,
                    }),
                    .value = i,
                }),
            });

            generic_args[i] = base_type;
        }

        ssa_base = ExprSSAGenerator{ctx}.generate_as_reference(meta_for_stmt.range);
    } else if (auto tuple_type = sequence_type.match<sir::TupleExpr>()) {
        values = arena.allocate_array<sir::Expr>(tuple_type->exprs.size());
        generic_args = arena.allocate_array<sir::Expr>(tuple_type->exprs.size());

        for (unsigned i = 0; i < values.size(); i++) {
            values[i] = arena.create<sir::FieldExpr>({
                .ast_node = nullptr,
                .type = tuple_type->exprs[i],
                .base = meta_for_stmt.range,
                .field_index = i,
            });

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

            sir::Expr string_type = arena.create<sir::PointerType>({
                .ast_node = nullptr,
                .base_type = arena.create(sir::PrimitiveType{.ast_node = nullptr, .primitive = sir::Primitive::U8}),
            });

            for (unsigned i = 0; i < values.size(); i++) {
                sir::Expr string_literal = arena.create<sir::StringLiteral>({
                    .ast_node = nullptr,
                    .type = string_type,
                    .value = struct_def->fields[i]->ident.value,
                });

                sir::Expr field_expr = arena.create<sir::FieldExpr>({
                    .ast_node = nullptr,
                    .type = struct_def->fields[i]->type,
                    .base = base,
                    .field_index = i,
                });

                sir::Expr tuple_type = arena.create<sir::TupleExpr>({
                    .ast_node = nullptr,
                    .type = nullptr,
                    .exprs = arena.create_array({string_type, struct_def->fields[i]->type}),
                });

                values[i] = arena.create<sir::TupleExpr>({
                    .ast_node = nullptr,
                    .type = tuple_type,
                    .exprs = arena.create_array({string_literal, field_expr}),
                });

                generic_args[i] = struct_def->fields[i]->type;
            }

            ssa_base = ExprSSAGenerator{ctx}.generate_as_reference(base);
        }
    }

    std::vector<SpecializationCollector::Entry> &specializations =
        ctx.specializations.meta_for_entries.at(&meta_for_stmt);

    for (unsigned i = 0; i < values.size(); i++) {
        StoredValue dst = ExprSSAGenerator{ctx}.generate_as_reference(values[i]);
        ctx.ssa_local_regs[&meta_for_stmt.local] = dst.get_ptr().get_register();

        SpecializationCollector::Entry *specialization = nullptr;

        for (SpecializationCollector::Entry &candidate : specializations) {
            if (candidate.args[0] == generic_args[i]) {
                specialization = &candidate;
                break;
            }
        }

        ASSERT(specialization);

        sir::Block &block = *std::get<sir::Block *>(meta_for_stmt.block);

        ctx.push_specialization(*specialization);
        generate_block_allocas(block, &meta_for_stmt.local);
        generate_block_body(block);
        ctx.pop_specialization(*specialization);
    }
}

void BlockSSAGenerator::generate_expr_stmt(const sir::Expr &expr) {
    ExprSSAGenerator(ctx).generate(expr, StorageHints::unused());
    DeinitSSAGenerator{ctx}.generate_deferred_deinits();
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

} // namespace banjo
