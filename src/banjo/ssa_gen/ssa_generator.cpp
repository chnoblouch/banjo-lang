#include "ssa_generator.hpp"

#include "banjo/ir/function_decl.hpp"
#include "banjo/ir/primitive.hpp"
#include "banjo/ir/virtual_register.hpp"
#include "banjo/ir_builder/ir_builder_utils.hpp"
#include "banjo/sir/sir.hpp"
#include "banjo/sir/sir_visitor.hpp"
#include "banjo/ssa_gen/block_ssa_generator.hpp"
#include "banjo/ssa_gen/global_ssa_generator.hpp"
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
        ctx.push_decl_context().sir_mod = sir_mod;
        create_decls(sir_mod->block);
        ctx.pop_decl_context();
    }

    for (const sir::Module *sir_mod : sir_unit.mods) {
        generate_decls(sir_mod->block);
    }

    return std::move(ssa_mod);
}

void SSAGenerator::create_decls(const sir::DeclBlock &decl_block) {
    for (const sir::Decl &decl : decl_block.decls) {
        if (auto func_def = decl.match<sir::FuncDef>()) create_func_def(*func_def);
        else if (auto native_func_decl = decl.match<sir::NativeFuncDecl>()) create_native_func_decl(*native_func_decl);
        else if (auto struct_def = decl.match<sir::StructDef>()) create_struct_def(*struct_def);
        else if (auto union_def = decl.match<sir::UnionDef>()) create_union_def(*union_def);
        else if (auto var_decl = decl.match<sir::VarDecl>()) create_var_decl(*var_decl);
        else if (auto native_var_decl = decl.match<sir::NativeVarDecl>()) create_native_var_decl(*native_var_decl);
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

    SSAGeneratorContext::DeclContext &decl_context = ctx.push_decl_context();
    decl_context.sir_struct_def = &sir_struct_def;
    decl_context.sir_generic_args = generic_args;
    create_decls(sir_struct_def.block);
    ctx.pop_decl_context();
}

void SSAGenerator::create_union_def(const sir::UnionDef &sir_union_def) {
    create_decls(sir_union_def.block);
}

void SSAGenerator::create_var_decl(const sir::VarDecl &sir_var_decl) {
    ctx.ssa_globals.insert({&sir_var_decl, ssa_mod.get_globals().size()});
    std::string name = sir_var_decl.ident.value;
    ssa_mod.add(ssa::Global(name, {}, {}));
}

void SSAGenerator::create_native_var_decl(const sir::NativeVarDecl &sir_native_var_decl) {
    ctx.ssa_extern_globals.insert({&sir_native_var_decl, ssa_mod.get_external_globals().size()});

    std::string name = sir_native_var_decl.ident.value;
    if (sir_native_var_decl.attrs && sir_native_var_decl.attrs->link_name) {
        name = *sir_native_var_decl.attrs->link_name;
    }

    ssa_mod.add(ssa::GlobalDecl(name, {}));
}

void SSAGenerator::generate_decls(const sir::DeclBlock &decl_block) {
    for (const sir::Decl &decl : decl_block.decls) {
        if (auto func_def = decl.match<sir::FuncDef>()) generate_func_def(*func_def);
        else if (auto struct_def = decl.match<sir::StructDef>()) generate_struct_def(*struct_def);
        else if (auto union_def = decl.match<sir::UnionDef>()) generate_union_def(*union_def);
        else if (auto var_decl = decl.match<sir::VarDecl>()) generate_var_decl(*var_decl);
        else if (auto native_var_decl = decl.match<sir::NativeVarDecl>()) generate_native_var_decl(*native_var_decl);
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

    BlockSSAGenerator(ctx).generate_block(sir_func.block);

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

void SSAGenerator::generate_union_def(const sir::UnionDef &sir_union_def) {
    generate_decls(sir_union_def.block);
}

void SSAGenerator::generate_var_decl(const sir::VarDecl &sir_var_decl) {
    ssa::Global &ssa_global = ssa_mod.get_globals()[ctx.ssa_globals[&sir_var_decl]];

    ssa::Type type = TypeSSAGenerator(ctx).generate(sir_var_decl.type);
    ssa_global.set_type(type);

    if (sir_var_decl.value) {
        ssa_global.initial_value = GlobalSSAGenerator(ctx).generate_value(sir_var_decl.value);
    }
}

void SSAGenerator::generate_native_var_decl(const sir::NativeVarDecl &sir_native_var_decl) {
    ssa::Type ssa_type = TypeSSAGenerator(ctx).generate(sir_native_var_decl.type);
    ssa_mod.get_external_globals()[ctx.ssa_extern_globals[&sir_native_var_decl]].set_type(ssa_type);
}

} // namespace lang

} // namespace banjo
