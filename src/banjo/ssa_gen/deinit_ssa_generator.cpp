#include "deinit_ssa_generator.hpp"

#include "banjo/sir/magic_methods.hpp"
#include "banjo/ssa_gen/expr_ssa_generator.hpp"
#include "banjo/ssa_gen/type_ssa_generator.hpp"

namespace banjo {

DeinitSSAGenerator::DeinitSSAGenerator(SSAGeneratorContext &ctx) : ctx{ctx} {}

void DeinitSSAGenerator::generate_deferred_deinits() {
    std::optional<ssa::Instruction> branch_instr;

    if (ctx.get_ssa_block()->is_branching()) {
        branch_instr = *ctx.get_ssa_block()->get_exit_iter();
        ctx.get_ssa_block()->remove(ctx.get_ssa_block()->get_exit_iter());
    }

    for (DeferredDeinit &deferred_deinit : ctx.get_func_context().cur_deferred_deinits) {
        generate_deinit(*deferred_deinit.resource, deferred_deinit.ssa_ptr);
    }

    ctx.get_func_context().cur_deferred_deinits.clear();

    if (branch_instr) {
        ctx.get_ssa_block()->append(*std::move(branch_instr));
    }
}

void DeinitSSAGenerator::generate_deinit(const sir::Resource &resource, sir::Symbol symbol) {
    sir::Expr type = symbol.get_type();

    sir::SymbolExpr ptr{
        .ast_node = nullptr,
        .type = type,
        .symbol = symbol,
    };

    ssa::Value ssa_ptr = ExprSSAGenerator{ctx}.generate_as_reference(&ptr).get_ptr();
    generate_deinit(resource, ssa_ptr);
}

void DeinitSSAGenerator::generate_deinit(const sir::Resource &resource, ssa::Value ssa_ptr) {
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

void DeinitSSAGenerator::generate_deinit_call(const sir::Resource &resource, ssa::Value ssa_ptr) {
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

    ssa::Function *ssa_func = ctx.ssa_funcs.find({&deinit_symbol.as<sir::FuncDef>(), generic_args});
    ssa::Value ssa_callee = ssa::Operand::from_func(ssa_func, ssa::Primitive::ADDR);
    ctx.get_ssa_block()->append({ssa::Opcode::CALL, {ssa_callee, std::move(ssa_ptr)}});
}

} // namespace banjo
