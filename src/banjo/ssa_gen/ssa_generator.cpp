#include "ssa_generator.hpp"

#include "banjo/ir/function_decl.hpp"
#include "banjo/ir/primitive.hpp"
#include "banjo/ir/virtual_register.hpp"
#include "banjo/ir_builder/ir_builder_utils.hpp"
#include "banjo/sir/sir.hpp"
#include "banjo/sir/sir_visitor.hpp"
#include "banjo/ssa_gen/expr_ssa_generator.hpp"
#include "banjo/ssa_gen/name_mangling.hpp"
#include "banjo/ssa_gen/ssa_generator_context.hpp"
#include "banjo/ssa_gen/type_ssa_generator.hpp"
#include "banjo/utils/macros.hpp"

#include <utility>

namespace banjo {

namespace lang {

const ssa::Type SSA_MAIN_RETURN_TYPE = ssa::Primitive::I32;
const ssa::Value SSA_MAIN_RETURN_VALUE = ssa::Value::from_int_immediate(0, SSA_MAIN_RETURN_TYPE);

SSAGenerator::SSAGenerator(const sir::Unit &sir_unit, target::Target *target) : sir_unit(sir_unit), ctx(target) {}

ssa::Module SSAGenerator::generate() {
    ctx.ssa_mod = &ssa_mod;

    for (const sir::Module *sir_mod : sir_unit.mods) {
        create_types(sir_mod->block);
    }

    for (const sir::Module *sir_mod : sir_unit.mods) {
        ctx.push_decl_context().sir_mod = sir_mod;
        create_decls(sir_mod->block);
        ctx.pop_decl_context();
    }

    for (const sir::Module *sir_mod : sir_unit.mods) {
        generate_decls(sir_mod->block);
    }

    return std::move(ssa_mod);
}

void SSAGenerator::create_types(const sir::DeclBlock &decl_block) {
    for (const sir::Decl &decl : decl_block.decls) {
        if (auto struct_def = decl.match<sir::StructDef>()) create_struct_type(*struct_def);
    }
}

void SSAGenerator::create_struct_type(const sir::StructDef &sir_struct_def) {
    if (sir_struct_def.is_generic()) {
        for (const sir::Specialization<sir::StructDef> &sir_specialization : sir_struct_def.specializations) {
            create_struct_type(*sir_specialization.def);
        }

        return;
    }

    ssa::Structure *ssa_struct = new ssa::Structure(sir_struct_def.ident.value);
    ssa_mod.add(ssa_struct);
    ctx.ssa_structs.insert({&sir_struct_def, ssa_struct});

    create_types(sir_struct_def.block);
}

void SSAGenerator::create_decls(const sir::DeclBlock &decl_block) {
    for (const sir::Decl &decl : decl_block.decls) {
        if (auto func_def = decl.match<sir::FuncDef>()) create_func_def(*func_def);
        else if (auto native_func_decl = decl.match<sir::NativeFuncDecl>()) create_native_func_decl(*native_func_decl);
        else if (auto struct_def = decl.match<sir::StructDef>()) create_struct_def(*struct_def);
    }
}

void SSAGenerator::create_func_def(const sir::FuncDef &sir_func) {
    if (sir_func.is_generic()) {
        for (const sir::Specialization<sir::FuncDef> &sir_specialization : sir_func.specializations) {
            create_func_def(*sir_specialization.def);
        }

        return;
    }

    sir::Attributes *attrs = sir_func.attrs;

    std::string ssa_name = NameMangling::get_link_name(ctx, sir_func);
    std::vector<ssa::Type> ssa_params = generate_params(sir_func.type);
    ssa::CallingConv ssa_calling_conv = ctx.target->get_default_calling_conv();

    ssa::Type ssa_return_type = generate_return_type(sir_func.type.return_type);
    if (sir_func.is_main() && ssa_return_type.is_primitive(ssa::Primitive::VOID)) {
        ssa_return_type = SSA_MAIN_RETURN_TYPE;
    }

    ssa::Function *ssa_func = new ssa::Function(ssa_name, ssa_params, ssa_return_type, ssa_calling_conv);
    ssa_func->set_global(sir_func.is_main() || (attrs && (attrs->exposed || attrs->dllexport)));

    ssa_mod.add(ssa_func);
    ctx.ssa_funcs.insert({&sir_func, ssa_func});

    if (attrs && attrs->dllexport && ctx.target->get_descr().is_windows()) {
        ssa_mod.add_dll_export(ssa_name);
    }
}

void SSAGenerator::create_native_func_decl(const sir::NativeFuncDecl &sir_func) {
    std::string ssa_name = NameMangling::get_link_name(sir_func);
    std::vector<ssa::Type> ssa_params = generate_params(sir_func.type);
    ssa::Type ssa_return_type = generate_return_type(sir_func.type.return_type);
    ssa::CallingConv ssa_calling_conv = ctx.target->get_default_calling_conv();
    ssa_mod.add(ssa::FunctionDecl(ssa_name, ssa_params, ssa_return_type, ssa_calling_conv));
}

std::vector<ssa::Type> SSAGenerator::generate_params(const sir::FuncType &sir_func_type) {
    unsigned sir_num_params = sir_func_type.params.size();

    ssa::Type ssa_return_type = TypeSSAGenerator(ctx).generate(sir_func_type.return_type);
    ReturnMethod return_method = ctx.get_return_method(ssa_return_type);
    bool has_return_pointer_arg = return_method == ReturnMethod::VIA_POINTER_ARG;

    std::vector<ssa::Type> ssa_params;
    ssa_params.reserve(has_return_pointer_arg ? (sir_num_params + 1) : sir_num_params);

    if (has_return_pointer_arg) {
        ssa_params.push_back(ssa::Primitive::ADDR);
    }

    for (const sir::Param &sir_param : sir_func_type.params) {
        ssa::Type ssa_param_type = TypeSSAGenerator(ctx).generate(sir_param.type);

        if (ctx.target->get_data_layout().fits_in_register(ssa_param_type)) {
            ssa_params.push_back(ssa_param_type);
        } else {
            ssa_params.push_back(ssa::Primitive::ADDR);
        }
    }

    return ssa_params;
}

ssa::Type SSAGenerator::generate_return_type(const sir::Expr &sir_return_type) {
    ssa::Type ssa_return_type = TypeSSAGenerator(ctx).generate(sir_return_type);
    ReturnMethod return_method = ctx.get_return_method(ssa_return_type);

    if (return_method == ReturnMethod::VIA_POINTER_ARG) {
        ssa_return_type = ssa::Primitive::VOID;
    }

    return ssa_return_type;
}

void SSAGenerator::create_struct_def(
    const sir::StructDef &sir_struct_def,
    const std::vector<sir::Expr> *generic_args /* = nullptr */
) {
    if (sir_struct_def.is_generic()) {
        for (const sir::Specialization<sir::StructDef> &sir_specialization : sir_struct_def.specializations) {
            create_struct_def(*sir_specialization.def, &sir_specialization.args);
        }

        return;
    }

    ssa::Structure *ssa_struct = ctx.ssa_structs[&sir_struct_def];

    for (sir::StructField *sir_field : sir_struct_def.fields) {
        ssa_struct->add({
            .name = sir_field->ident.value,
            .type = TypeSSAGenerator(ctx).generate(sir_field->type),
        });
    }

    SSAGeneratorContext::DeclContext &decl_context = ctx.push_decl_context();
    decl_context.sir_struct_def = &sir_struct_def;
    decl_context.sir_generic_args = generic_args;

    create_decls(sir_struct_def.block);

    ctx.pop_decl_context();
}

void SSAGenerator::generate_decls(const sir::DeclBlock &decl_block) {
    for (const sir::Decl &decl : decl_block.decls) {
        if (auto func_def = decl.match<sir::FuncDef>()) generate_func_def(*func_def);
        else if (auto struct_def = decl.match<sir::StructDef>()) generate_struct_def(*struct_def);
    }
}

void SSAGenerator::generate_func_def(const sir::FuncDef &sir_func) {
    if (sir_func.is_generic()) {
        for (const sir::Specialization<sir::FuncDef> &sir_specialization : sir_func.specializations) {
            generate_func_def(*sir_specialization.def);
        }

        return;
    }

    ssa::Function *ssa_func = ctx.ssa_funcs[&sir_func];
    ctx.push_func_context(ssa_func);
    ctx.get_func_context().ssa_func_exit = ctx.create_block();

    for (unsigned i = 0; i < ssa_func->get_params().size(); i++) {
        const ssa::Type &ssa_param_type = ssa_func->get_params()[i];
        ssa::VirtualRegister ssa_slot = ssa_func->next_virtual_reg();
        ssa::Instruction &alloca_instr = ctx.append_alloca(ssa_slot, ssa_param_type);
        alloca_instr.set_flag(ssa::Instruction::FLAG_ARG_STORE);

        ssa::VirtualRegister ssa_arg_val = ssa_func->next_virtual_reg();
        ssa::Instruction &loadarg_instr = ctx.append_loadarg(ssa_arg_val, ssa_param_type, i);
        loadarg_instr.set_flag(ssa::Instruction::FLAG_SAVE_ARG);

        ssa::Instruction &store_instr = ctx.append_store(
            ssa::Operand::from_register(ssa_arg_val, ssa_param_type),
            ssa::Operand::from_register(ssa_slot, ssa::Primitive::ADDR)
        );
        store_instr.set_flag(ssa::Instruction::FLAG_SAVE_ARG);

        ctx.get_func_context().arg_regs.push_back(ssa_slot);
    }

    ssa::Type ssa_return_type = TypeSSAGenerator(ctx).generate(sir_func.type.return_type);
    ReturnMethod return_method = ctx.get_return_method(ssa_return_type);
    unsigned param_mapping_offset = return_method == ReturnMethod::VIA_POINTER_ARG ? 1 : 0;

    if (return_method != ReturnMethod::NO_RETURN_VALUE) {
        ctx.get_func_context().ssa_return_slot = ctx.append_alloca(ssa_return_type);
    }

    for (unsigned i = 0; i < sir_func.type.params.size(); i++) {
        const sir::Param &sir_param = sir_func.type.params[i];
        ssa::VirtualRegister ssa_slot = ctx.get_func_context().arg_regs[i + param_mapping_offset];
        ctx.ssa_param_slots.insert({&sir_param, ssa_slot});
    }

    generate_block(sir_func.block);

    if (!ir_builder::IRBuilderUtils::is_branching(*ctx.get_ssa_block())) {
        ctx.append_jmp(ctx.get_func_context().ssa_func_exit);
    }

    ctx.append_block(ctx.get_func_context().ssa_func_exit);

    if (return_method == ReturnMethod::NO_RETURN_VALUE) {
        if (sir_func.is_main()) {
            ctx.append_ret(SSA_MAIN_RETURN_VALUE);
        } else {
            ctx.append_ret();
        }
    } else if (return_method == ReturnMethod::IN_REGISTER) {
        ssa::VirtualRegister ssa_return_slot = ctx.get_func_context().ssa_return_slot;
        ssa::Value ssa_return_val = ctx.append_load(ssa_return_type, ssa_return_slot);
        ctx.append_ret(ssa_return_val);
    } else if (return_method == ReturnMethod::VIA_POINTER_ARG) {
        ssa::Value ssa_copy_dst = ctx.append_load(ssa::Primitive::ADDR, ctx.get_func_context().arg_regs[0]);
        ssa::VirtualRegister ssa_return_slot = ctx.get_func_context().ssa_return_slot;
        ssa::Value ssa_copy_src = ssa::Value::from_register(ssa_return_slot, ssa::Primitive::ADDR);
        ctx.append_copy(ssa_copy_dst, ssa_copy_src, ssa_return_type);
        ctx.append_ret();
    } else {
        ASSERT_UNREACHABLE;
    }

    ctx.pop_func_context();
}

void SSAGenerator::generate_struct_def(const sir::StructDef &sir_struct_def) {
    if (sir_struct_def.is_generic()) {
        for (const sir::Specialization<sir::StructDef> &sir_specialization : sir_struct_def.specializations) {
            generate_struct_def(*sir_specialization.def);
        }

        return;
    }

    generate_decls(sir_struct_def.block);
}

void SSAGenerator::generate_block(const sir::Block &sir_block) {
    for (const sir::Stmt &sir_stmt : sir_block.stmts) {
        if (ir_builder::IRBuilderUtils::is_branching(*ctx.get_ssa_block())) {
            continue;
        }

        SIR_VISIT_STMT(
            sir_stmt,
            SIR_VISIT_IMPOSSIBLE,
            generate_var_stmt(*inner),
            generate_assign_stmt(*inner),
            SIR_VISIT_IMPOSSIBLE,
            generate_return_stmt(*inner),
            generate_if_stmt(*inner),
            SIR_VISIT_IMPOSSIBLE,
            SIR_VISIT_IMPOSSIBLE,
            generate_loop_stmt(*inner),
            generate_continue_stmt(*inner),
            generate_break_stmt(*inner),
            SIR_VISIT_IGNORE,
            SIR_VISIT_IGNORE,
            ExprSSAGenerator(ctx).generate(*inner, StorageHints::unused()),
            generate_block(*inner)
        );
    }
}

void SSAGenerator::generate_var_stmt(const sir::VarStmt &var_stmt) {
    ssa::Type ssa_type = TypeSSAGenerator(ctx).generate(var_stmt.type);
    ssa::VirtualRegister reg = ctx.append_alloca(ssa_type);
    ctx.ssa_var_regs.insert({&var_stmt, reg});

    if (var_stmt.value) {
        ssa::Value ssa_ptr = ssa::Value::from_register(reg, ssa::Primitive::ADDR);
        ExprSSAGenerator(ctx).generate_into_dst(var_stmt.value, ssa_ptr);
    }
}

void SSAGenerator::generate_assign_stmt(const sir::AssignStmt &assign_stmt) {
    StoredValue dst = ExprSSAGenerator(ctx).generate(assign_stmt.lhs, StorageHints::prefer_reference());
    ExprSSAGenerator(ctx).generate_into_dst(assign_stmt.rhs, dst.get_ptr());
}

void SSAGenerator::generate_return_stmt(const sir::ReturnStmt &return_stmt) {
    if (return_stmt.value) {
        ExprSSAGenerator(ctx).generate_into_dst(return_stmt.value, ctx.get_func_context().ssa_return_slot);
    }

    ctx.append_jmp(ctx.get_func_context().ssa_func_exit);
}

void SSAGenerator::generate_if_stmt(const sir::IfStmt &if_stmt) {
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

void SSAGenerator::generate_loop_stmt(const sir::LoopStmt &loop_stmt) {
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

void SSAGenerator::generate_continue_stmt(const sir::ContinueStmt & /*continue_stmt*/) {
    ctx.append_jmp(ctx.get_loop_context().ssa_continue_target);
}

void SSAGenerator::generate_break_stmt(const sir::BreakStmt & /*break_stmt*/) {
    ctx.append_jmp(ctx.get_loop_context().ssa_break_target);
}

} // namespace lang

} // namespace banjo
