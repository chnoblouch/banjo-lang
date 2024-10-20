#include "block_ssa_generator.hpp"

#include "banjo/ir/comparison.hpp"
#include "banjo/ir/virtual_register.hpp"
#include "banjo/ir_builder/ir_builder_utils.hpp"
#include "banjo/sir/magic_methods.hpp"
#include "banjo/sir/sir.hpp"
#include "banjo/sir/sir_visitor.hpp"
#include "banjo/ssa_gen/expr_ssa_generator.hpp"
#include "banjo/ssa_gen/ssa_generator_context.hpp"
#include "banjo/ssa_gen/storage_hints.hpp"
#include "banjo/ssa_gen/stored_value.hpp"
#include "banjo/ssa_gen/type_ssa_generator.hpp"

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
            ctx.ssa_local_regs.insert({local, reg});
        }
    }

    for (const auto &[symbol, resource] : block.resources) {
        generate_resource_flags(resource);
    }
}

void BlockSSAGenerator::generate_resource_flags(const sir::Resource &resource) {
    if (resource.ownership == sir::Ownership::MOVED_COND) {
        ssa::VirtualRegister flag_slot = ctx.append_alloca(ssa::Primitive::I8);
        ctx.append_store(DEINIT_FLAG_TRUE, flag_slot);

        ctx.get_func_context().resource_deinit_flags.insert({&resource, flag_slot});
    }

    for (const auto &[field_index, sub_resource] : resource.sub_resources) {
        generate_resource_flags(sub_resource);
    }
}

void BlockSSAGenerator::generate_block_body(const sir::Block &block) {
    for (const sir::Stmt &sir_stmt : block.stmts) {
        if (ir_builder::IRBuilderUtils::is_branching(*ctx.get_ssa_block())) {
            return;
        }

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
            SIR_VISIT_IGNORE,                                               // meta_for_stmt
            SIR_VISIT_IGNORE,                                               // expanded_meta_stmt
            ExprSSAGenerator(ctx).generate(*inner, StorageHints::unused()), // expr_stmt
            generate_block(*inner),                                         // block_stmt
            SIR_VISIT_IMPOSSIBLE                                            // error
        );

        for (DeferredDeinit &deferred_deinit : ctx.get_func_context().cur_deferred_deinits) {
            generate_deinit(*deferred_deinit.resource, deferred_deinit.ssa_ptr);
        }

        ctx.get_func_context().cur_deferred_deinits.clear();
    }

    generate_block_deinit(block);
}

void BlockSSAGenerator::generate_block_deinit(const sir::Block &block) {
    for (const auto &[symbol, resource] : block.resources) {
        generate_deinit(resource, symbol);
    }
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
    if (return_stmt.value) {
        ExprSSAGenerator(ctx).generate_into_dst(return_stmt.value, ctx.get_func_context().ssa_return_slot);
    }

    ctx.append_jmp(ctx.get_func_context().ssa_func_exit);
}

void BlockSSAGenerator::generate_if_stmt(const sir::IfStmt &if_stmt) {
    ssa::BasicBlockIter ssa_end_block = ctx.create_block();

    for (unsigned i = 0; i < if_stmt.cond_branches.size(); i++) {
        const sir::IfCondBranch &sir_branch = if_stmt.cond_branches[i];
        bool is_final_branch = i == if_stmt.cond_branches.size() - 1 && !if_stmt.else_branch;

        ssa::BasicBlockIter ssa_next_block = is_final_branch ? nullptr : ctx.create_block();
        ssa::BasicBlockIter ssa_target_if_true = ctx.create_block();
        ssa::BasicBlockIter ssa_target_if_false = is_final_branch ? ssa_end_block : ssa_next_block;

        ExprSSAGenerator(ctx).generate_branch(sir_branch.condition, {ssa_target_if_true, ssa_target_if_false});

        ctx.append_block(ssa_target_if_true);
        generate_block(sir_branch.block);
        ctx.append_jmp(ssa_end_block);

        if (!is_final_branch) {
            ctx.append_block(ssa_next_block);
        }
    }

    if (if_stmt.else_branch) {
        generate_block(if_stmt.else_branch->block);
        ctx.append_jmp(ssa_end_block);
    }

    ctx.append_block(ssa_end_block);
}

void BlockSSAGenerator::generate_switch_stmt(const sir::SwitchStmt &switch_stmt) {
    const sir::UnionDef &sir_union_def = switch_stmt.value.get_type().as_symbol<sir::UnionDef>();

    ssa::BasicBlockIter ssa_end_block = ctx.create_block();

    StoredValue ssa_value = ExprSSAGenerator(ctx).generate(switch_stmt.value);
    ssa::VirtualRegister ssa_tag_ptr_reg = ctx.append_memberptr(ssa_value.value_type, ssa_value.get_ptr(), 0);
    ssa::Value ssa_tag = ctx.append_load(ssa::Primitive::I32, ssa_tag_ptr_reg);

    for (unsigned i = 0; i < switch_stmt.case_branches.size(); i++) {
        const sir::SwitchCaseBranch &sir_branch = switch_stmt.case_branches[i];
        const sir::UnionCase &sir_union_case = sir_branch.local.type.as_symbol<sir::UnionCase>();
        unsigned tag = sir_union_def.get_index(sir_union_case);
        bool is_final_branch = i == switch_stmt.case_branches.size() - 1;

        ssa::BasicBlockIter ssa_next_block = is_final_branch ? nullptr : ctx.create_block();
        ssa::BasicBlockIter ssa_target_if_true = ctx.create_block();
        ssa::BasicBlockIter ssa_target_if_false = is_final_branch ? ssa_end_block : ssa_next_block;

        ssa::Value ssa_tag_value = ssa::Value::from_int_immediate(tag, ssa::Primitive::I32);
        ctx.append_cjmp(ssa_tag, ssa::Comparison::EQ, ssa_tag_value, ssa_target_if_true, ssa_target_if_false);
        ctx.append_block(ssa_target_if_true);

        generate_block_allocas(sir_branch.block);

        ssa::Type ssa_case_type = TypeSSAGenerator(ctx).generate(sir_branch.local.type);
        ssa::VirtualRegister ssa_data_ptr_reg = ctx.append_memberptr(ssa_value.value_type, ssa_value.get_ptr(), 1);
        StoredValue ssa_data_ptr = StoredValue::create_reference(ssa_data_ptr_reg, ssa_case_type);
        ssa::VirtualRegister ssa_local_reg = ctx.ssa_local_regs[&sir_branch.local];
        ssa_data_ptr.copy_to(ssa_local_reg, ctx);

        generate_block_body(sir_branch.block);
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
        .ssa_continue_target = loop_stmt.latch ? ssa_latch_block : ssa_cond_block,
        .ssa_break_target = ssa_end_block,
    });

    ctx.append_block(ssa_body_entry_block);
    generate_block(loop_stmt.block);

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
    ctx.append_jmp(ctx.get_loop_context().ssa_continue_target);
}

void BlockSSAGenerator::generate_break_stmt(const sir::BreakStmt & /*break_stmt*/) {
    ctx.append_jmp(ctx.get_loop_context().ssa_break_target);
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
    if (resource.ownership == sir::Ownership::OWNED) {
        generate_deinit_call(resource, std::move(ssa_ptr));
    } else if (resource.ownership == sir::Ownership::MOVED_COND) {
        ssa::BasicBlockIter deinit_block = ctx.create_block();
        ssa::BasicBlockIter end_block = ctx.create_block();

        ssa::VirtualRegister flag_slot = ctx.get_func_context().resource_deinit_flags.at(&resource);
        ssa::Value flag_val = ctx.append_load(ssa::Primitive::I8, flag_slot);
        ctx.append_cjmp(flag_val, ssa::Comparison::NE, DEINIT_FLAG_FALSE, deinit_block, end_block);

        ctx.append_block(deinit_block);
        generate_deinit_call(resource, std::move(ssa_ptr));
        ctx.append_jmp(end_block);
        ctx.append_block(end_block);
    }

    if (resource.has_deinit) {
        const std::string &name = resource.type.as_symbol<sir::StructDef>().ident.value;
        if (name == "Result" || name == "Optional") {
            return;
        }
    }

    ssa::Type ssa_type = TypeSSAGenerator(ctx).generate(resource.type);

    for (const auto &[field_index, sub_resource] : resource.sub_resources) {
        ssa::VirtualRegister field_ptr_reg = ctx.append_memberptr(ssa_type, ssa_ptr, field_index);
        ssa::Value field_ptr = ssa::Value::from_register(field_ptr_reg, ssa::Primitive::ADDR);
        generate_deinit(sub_resource, field_ptr);
    }
}

void BlockSSAGenerator::generate_deinit_call(const sir::Resource &resource, ssa::Value ssa_ptr) {
    if (!resource.has_deinit) {
        return;
    }

    sir::SymbolTable *symbol_table = resource.type.as_symbol<sir::StructDef>().block.symbol_table;
    sir::Symbol deinit_symbol = symbol_table->look_up(sir::MagicMethods::DEINIT);

    sir::SymbolExpr callee{
        .ast_node = nullptr,
        .type = deinit_symbol.get_type(),
        .symbol = deinit_symbol,
    };

    ssa::Value ssa_callee = ExprSSAGenerator(ctx).generate(&callee).get_value();
    ctx.get_ssa_block()->append({ssa::Opcode::CALL, {ssa_callee, std::move(ssa_ptr)}});
}

} // namespace lang

} // namespace banjo
