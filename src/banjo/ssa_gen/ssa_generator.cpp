#include "ssa_generator.hpp"

#include "banjo/ir/function_decl.hpp"
#include "banjo/ir/primitive.hpp"
#include "banjo/ir/virtual_register.hpp"
#include "banjo/ir_builder/ir_builder_utils.hpp"
#include "banjo/sir/sir.hpp"
#include "banjo/ssa_gen/expr_ssa_generator.hpp"
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

    for (const sir::Decl &decl : sir_unit.block.decls) {
        if (auto func_def = decl.match<sir::FuncDef>()) create_func_def(*func_def);
        else if (auto native_func_decl = decl.match<sir::NativeFuncDecl>()) create_native_func_decl(*native_func_decl);
        else if (auto struct_def = decl.match<sir::StructDef>()) create_struct_def(*struct_def);
    }

    for (const sir::Decl &decl : sir_unit.block.decls) {
        if (auto func_def = decl.match<sir::FuncDef>()) generate_func_def(*func_def);
        else if (auto struct_def = decl.match<sir::StructDef>()) generate_struct_def(*struct_def);
    }

    return std::move(ssa_mod);
}

void SSAGenerator::create_func_def(const sir::FuncDef &sir_func) {
    std::string ssa_name = sir_func.ident.value;
    std::vector<ssa::Type> ssa_params = generate_params(sir_func.type);
    ssa::CallingConv ssa_calling_conv = ctx.target->get_default_calling_conv();

    ssa::Type ssa_return_type = generate_return_type(sir_func.type.return_type);
    if (sir_func.is_main() && ssa_return_type.is_primitive(ssa::Primitive::VOID)) {
        ssa_return_type = SSA_MAIN_RETURN_TYPE;
    }

    ssa::Function *ssa_func = new ssa::Function(ssa_name, ssa_params, ssa_return_type, ssa_calling_conv);
    ssa_func->set_global(true);

    ssa_mod.add(ssa_func);
    ctx.ssa_funcs.insert({&sir_func, ssa_func});
}

void SSAGenerator::create_native_func_decl(const sir::NativeFuncDecl &sir_func) {
    std::string ssa_name = sir_func.ident.value;
    std::vector<ssa::Type> ssa_params = generate_params(sir_func.type);
    ssa::Type ssa_return_type = generate_return_type(sir_func.type.return_type);
    ssa::CallingConv ssa_calling_conv = ctx.target->get_default_calling_conv();

    ssa_mod.add(ssa::FunctionDecl(sir_func.ident.value, ssa_params, ssa_return_type, ssa_calling_conv));
}

std::vector<ssa::Type> SSAGenerator::generate_params(const sir::FuncType &sir_func_type) {
    unsigned sir_num_params = sir_func_type.params.size();

    ssa::Type ssa_return_type = TypeSSAGenerator(ctx).generate(sir_func_type.return_type);
    ReturnMethod return_method = ctx.get_return_method(ssa_return_type);
    bool has_return_pointer_arg = return_method != ReturnMethod::VIA_POINTER_ARG;

    std::vector<ssa::Type> ssa_params;
    ssa_params.reserve(has_return_pointer_arg ? (sir_num_params + 1) : sir_num_params);

    if (has_return_pointer_arg) {
        ssa_params.push_back(ssa::Primitive::ADDR);
    }

    for (const sir::Param &sir_param : sir_func_type.params) {
        ssa_params.push_back(TypeSSAGenerator(ctx).generate(sir_param.type));
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

void SSAGenerator::create_struct_def(const sir::StructDef &sir_struct) {
    ssa::Structure *ssa_struct = new ssa::Structure(sir_struct.ident.value);
    ssa_mod.add(ssa_struct);
    ctx.ssa_structs.insert({&sir_struct, ssa_struct});
}

void SSAGenerator::generate_func_def(const sir::FuncDef &sir_func) {
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
    ssa::Structure *ssa_struct = ctx.ssa_structs[&sir_struct_def];

    for (sir::StructField *sir_field : sir_struct_def.fields) {
        ssa_struct->add({
            .name = sir_field->ident.value,
            .type = TypeSSAGenerator(ctx).generate(sir_field->type),
        });
    }
}

void SSAGenerator::generate_block(const sir::Block &sir_block) {
    for (const sir::Stmt &sir_stmt : sir_block.stmts) {
        if (auto var_stmt = sir_stmt.match<sir::VarStmt>()) generate_var_stmt(*var_stmt);
        else if (auto assign_stmt = sir_stmt.match<sir::AssignStmt>()) generate_assign_stmt(*assign_stmt);
        else if (auto return_stmt = sir_stmt.match<sir::ReturnStmt>()) generate_return_stmt(*return_stmt);
        else if (auto if_stmt = sir_stmt.match<sir::IfStmt>()) generate_if_stmt(*if_stmt);
        else if (auto expr = sir_stmt.match<sir::Expr>()) ExprSSAGenerator(ctx).generate(*expr, StorageHints::unused());
        else ASSERT_UNREACHABLE;
    }
}

void SSAGenerator::generate_var_stmt(const sir::VarStmt &var_stmt) {
    ssa::Type ssa_type = TypeSSAGenerator(ctx).generate(var_stmt.type);
    ssa::VirtualRegister reg = ctx.append_alloca(ssa_type);
    ctx.ssa_var_regs.insert({&var_stmt, reg});

    ssa::Value ssa_ptr = ssa::Value::from_register(reg, ssa::Primitive::ADDR);
    ExprSSAGenerator(ctx).generate_into_dst(var_stmt.value, ssa_ptr);
}

void SSAGenerator::generate_assign_stmt(const sir::AssignStmt &assign_stmt) {
    StoredValue dst = ExprSSAGenerator(ctx).generate(assign_stmt.lhs, StorageHints::prefer_reference());
    ExprSSAGenerator(ctx).generate_into_dst(assign_stmt.rhs, dst.get_ptr());
}

void SSAGenerator::generate_return_stmt(const sir::ReturnStmt &return_stmt) {
    ExprSSAGenerator(ctx).generate_into_dst(return_stmt.value, ctx.get_func_context().ssa_return_slot);
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

        ExprSSAGenerator(ctx).generate_bool_expr(sir_branch.condition, {ssa_target_if_true, ssa_target_if_false});

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

} // namespace lang

} // namespace banjo
