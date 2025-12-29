#include "ssa_generator.hpp"

#include "banjo/sir/sir.hpp"
#include "banjo/sir/sir_visitor.hpp"
#include "banjo/ssa/function.hpp"
#include "banjo/ssa/primitive.hpp"
#include "banjo/ssa/structure.hpp"
#include "banjo/ssa/virtual_register.hpp"
#include "banjo/ssa_gen/block_ssa_generator.hpp"
#include "banjo/ssa_gen/global_ssa_generator.hpp"
#include "banjo/ssa_gen/name_mangling.hpp"
#include "banjo/ssa_gen/ssa_generator_context.hpp"
#include "banjo/ssa_gen/type_ssa_generator.hpp"
#include "banjo/target/target_data_layout.hpp"
#include "banjo/utils/macros.hpp"
#include "banjo/utils/timing.hpp"

#include <utility>

namespace banjo {

namespace lang {

const ssa::Type SSA_MAIN_ARGC_TYPE = ssa::Primitive::I32;
const ssa::Type SSA_MAIN_ARGV_TYPE = ssa::Primitive::ADDR;
const ssa::Type SSA_MAIN_RETURN_TYPE = ssa::Primitive::I32;
const ssa::Value SSA_MAIN_RETURN_VALUE = ssa::Value::from_int_immediate(0, SSA_MAIN_RETURN_TYPE);

SSAGenerator::SSAGenerator(const sir::Unit &sir_unit, target::Target *target) : sir_unit(sir_unit), ctx(target) {}

ssa::Module SSAGenerator::generate() {
    PROFILE_SCOPE("ssa generator");

    ctx.ssa_mod = &ssa_mod;

    for (const sir::Module *sir_mod : sir_unit.mods) {
        ctx.push_decl_context().sir_mod = sir_mod;
        create_decls(sir_mod->block);
        ctx.pop_decl_context();
    }

    generate_runtime();

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
        else if (auto proto_def = decl.match<sir::ProtoDef>()) create_proto_def(*proto_def);
        else if (auto var_decl = decl.match<sir::VarDecl>()) create_var_decl(*var_decl);
        else if (auto native_var_decl = decl.match<sir::NativeVarDecl>()) create_native_var_decl(*native_var_decl);
    }
}

void SSAGenerator::create_func_def(const sir::FuncDef &sir_func) {
    if (sir_func.parent.is<sir::ProtoDef>() && sir_func.is_method()) {
        return;
    }

    if (sir_func.is_generic()) {
        for (const sir::Specialization<sir::FuncDef> &sir_specialization : sir_func.specializations) {
            create_func_def(*sir_specialization.def);
        }

        return;
    }

    sir::Attributes *attrs = sir_func.attrs;

    std::string ssa_name = NameMangling::get_link_name(sir_func);

    ssa::FunctionType ssa_func_type{
        .params = generate_params(sir_func.type),
        .return_type = generate_return_type(sir_func.type.return_type),
        .calling_conv = ctx.target->get_default_calling_conv(),
    };

    if (sir_func.is_main()) {
        if (ssa_func_type.params.empty()) {
            ssa_func_type.params = {SSA_MAIN_ARGC_TYPE, SSA_MAIN_ARGV_TYPE};
        }

        if (ssa_func_type.return_type.is_primitive(ssa::Primitive::VOID)) {
            ssa_func_type.return_type = SSA_MAIN_RETURN_TYPE;
        }
    }

    ssa::Function *ssa_func = new ssa::Function(ssa_name, ssa_func_type);
    ssa_func->global = sir_func.is_main() || (attrs && (attrs->exposed || attrs->dllexport));

    ssa_mod.add(ssa_func);
    ctx.ssa_funcs.insert({&sir_func, ssa_func});

    if (attrs && attrs->dllexport && ctx.target->get_descr().is_windows()) {
        ssa_mod.add_dll_export(ssa_name);
    }
}

void SSAGenerator::create_native_func_decl(const sir::NativeFuncDecl &sir_func) {
    std::string ssa_name = NameMangling::get_link_name(sir_func);

    ssa::FunctionType ssa_func_type{
        .params = generate_params(sir_func.type),
        .return_type = generate_return_type(sir_func.type.return_type),
        .calling_conv = ctx.target->get_default_calling_conv(),
    };

    ssa::FunctionDecl *ssa_func = new ssa::FunctionDecl{
        .name = ssa_name,
        .type = ssa_func_type,
    };

    ssa_mod.add(ssa_func);
    ctx.ssa_native_funcs.insert({&sir_func, ssa_func});
}

std::vector<ssa::Type> SSAGenerator::generate_params(const sir::FuncType &sir_func_type) {
    target::TargetDataLayout &data_layout = ctx.target->get_data_layout();

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
        target::ArgPassMethod pass_method = data_layout.get_arg_pass_method(ssa_param_type);

        if (pass_method.via_pointer) {
            ssa_params.push_back(ssa::Primitive::ADDR);
        } else {
            for (unsigned i = 0; i < pass_method.num_args - 1; i++) {
                ssa_params.push_back(data_layout.get_usize_type());
            }

            ssa_params.push_back(pass_method.last_arg_type);
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
    const std::span<sir::Expr> *generic_args /* = nullptr */
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

void SSAGenerator::create_proto_def(const sir::ProtoDef &sir_proto_def) {
    ssa::Structure *vtable_type = new ssa::Structure("vtable." + std::string{sir_proto_def.ident.value});

    for (unsigned i = 0; i < sir_proto_def.func_decls.size(); i++) {
        vtable_type->add(ssa::StructureMember{
            .name = "f" + std::to_string(i),
            .type = ssa::Primitive::ADDR,
        });
    }

    ssa_mod.add(vtable_type);
    ctx.ssa_vtable_types[&sir_proto_def] = vtable_type;

    create_decls(sir_proto_def.block);
}

void SSAGenerator::create_var_decl(const sir::VarDecl &sir_var_decl) {
    ctx.ssa_globals.insert({&sir_var_decl, ssa_mod.get_globals().size()});

    ssa_mod.add(new ssa::Global{
        .name = std::string{sir_var_decl.ident.value},
        .type = {},
        .initial_value = {},
        .external = false,
    });
}

void SSAGenerator::create_native_var_decl(const sir::NativeVarDecl &sir_native_var_decl) {
    ctx.ssa_extern_globals.insert({&sir_native_var_decl, ssa_mod.get_external_globals().size()});

    std::string name{sir_native_var_decl.ident.value};
    if (sir_native_var_decl.attrs && sir_native_var_decl.attrs->link_name) {
        name = *sir_native_var_decl.attrs->link_name;
    }

    ssa_mod.add(new ssa::GlobalDecl{
        .name = name,
        .type = {},
    });
}

void SSAGenerator::generate_runtime() {
    ssa::Type usize_type = ctx.target->get_data_layout().get_usize_type();

    ssa::FunctionDecl *func_snprintf = new ssa::FunctionDecl{
        .name = "snprintf",
        .type{
            .params{ssa::Primitive::ADDR, usize_type, ssa::Primitive::ADDR},
            .return_type = ssa::Primitive::I32,
            .calling_conv = ctx.target->get_default_calling_conv(),
            .variadic = true,
        },
    };

    ssa::Global *global_format_string = new ssa::Global{
        .name = "___runtime_f64_format",
        .type = ssa::Primitive::ADDR,
        .initial_value = std::string("%g\0", 3),
        .external = false,
    };

    for (ssa::Function *func : ssa_mod.get_functions()) {
        if (func->name != "___runtime_f64_to_string") {
            continue;
        }

        func->get_entry_block().append({
            ssa::Opcode::LOADARG,
            0,
            {
                ssa::Operand::from_type(ssa::Primitive::F64),
                ssa::Operand::from_int_immediate(0),
            },
        });

        func->get_entry_block().append({
            ssa::Opcode::LOADARG,
            1,
            {
                ssa::Operand::from_type(ssa::Primitive::ADDR),
                ssa::Operand::from_int_immediate(1),
            },
        });

        func->get_entry_block().append({
            ssa::Opcode::LOADARG,
            2,
            {
                ssa::Operand::from_type(usize_type),
                ssa::Operand::from_int_immediate(2),
            },
        });

        ssa::Instruction call{
            ssa::Opcode::CALL,
            {
                ssa::Operand::from_extern_func(func_snprintf, ssa::Primitive::VOID),
                ssa::Operand::from_register(1, ssa::Primitive::ADDR),
                ssa::Operand::from_register(2, usize_type),
                ssa::Operand::from_global(global_format_string, ssa::Primitive::ADDR),
                ssa::Operand::from_register(0, ssa::Primitive::F64),
            }
        };

        call.set_attr(ssa::Instruction::Attribute::VARIADIC);
        call.set_attrs_data(3);

        func->get_entry_block().append(call);
        func->get_entry_block().append({ssa::Opcode::RET});

        func->last_virtual_reg = 3;
    }

    ssa_mod.add(func_snprintf);
    ssa_mod.add(global_format_string);
}

void SSAGenerator::generate_decls(const sir::DeclBlock &decl_block) {
    for (const sir::Decl &decl : decl_block.decls) {
        if (auto func_def = decl.match<sir::FuncDef>()) generate_func_def(*func_def);
        else if (auto struct_def = decl.match<sir::StructDef>()) generate_struct_def(*struct_def);
        else if (auto union_def = decl.match<sir::UnionDef>()) generate_union_def(*union_def);
        else if (auto proto_def = decl.match<sir::ProtoDef>()) generate_proto_def(*proto_def);
        else if (auto var_decl = decl.match<sir::VarDecl>()) generate_var_decl(*var_decl);
        else if (auto native_var_decl = decl.match<sir::NativeVarDecl>()) generate_native_var_decl(*native_var_decl);
    }
}

void SSAGenerator::generate_func_def(const sir::FuncDef &sir_func) {
    target::TargetDataLayout &data_layout = ctx.target->get_data_layout();

    if (sir_func.parent.is<sir::ProtoDef>() && sir_func.is_method()) {
        return;
    }

    if (sir_func.is_generic()) {
        for (const sir::Specialization<sir::FuncDef> &sir_specialization : sir_func.specializations) {
            generate_func_def(*sir_specialization.def);
        }

        return;
    }

    ssa::Function *ssa_func = ctx.ssa_funcs.at(&sir_func);
    if (ssa_func->name == "___runtime_f64_to_string") {
        return;
    }

    ctx.push_func_context(sir_func, ssa_func);
    ctx.get_func_context().ssa_func_exit = ctx.create_block();

    ssa::Type ssa_return_type = TypeSSAGenerator(ctx).generate(sir_func.type.return_type);
    ReturnMethod return_method = ctx.get_return_method(ssa_return_type);

    unsigned ssa_arg_index = 0;

    if (return_method == ReturnMethod::VIA_POINTER_ARG) {
        ssa::VirtualRegister ssa_slot = ssa_func->next_virtual_reg();

        ssa::Type ssa_arg_type = return_method == ReturnMethod::VIA_POINTER_ARG ? ssa::Primitive::ADDR : ssa_return_type;
        ssa::Instruction &alloca_instr = ctx.append_alloca(ssa_slot, ssa_arg_type);
        alloca_instr.set_attr(ssa::Instruction::Attribute::ARG_STORE);

        ssa::VirtualRegister ssa_arg_val = ssa_func->next_virtual_reg();
        ssa::Instruction &loadarg_instr = ctx.append_loadarg(ssa_arg_val, ssa_arg_type, ssa_arg_index);
        loadarg_instr.set_attr(ssa::Instruction::Attribute::SAVE_ARG);

        ssa::Instruction &store_instr = ctx.append_store(
            ssa::Operand::from_register(ssa_arg_val, ssa_arg_type),
            ssa::Operand::from_register(ssa_slot, ssa::Primitive::ADDR)
        );
        store_instr.set_attr(ssa::Instruction::Attribute::SAVE_ARG);

        ctx.get_func_context().ssa_return_arg_slot = ssa_slot;
        ssa_arg_index += 1;
    }

    for (const sir::Param &sir_param : sir_func.type.params) {
        const ssa::Type &ssa_param_type = TypeSSAGenerator(ctx).generate(sir_param.type);
        target::ArgPassMethod pass_method = data_layout.get_arg_pass_method(ssa_param_type);

        ssa::VirtualRegister ssa_slot = ssa_func->next_virtual_reg();

        ssa::Type alloca_type = pass_method.via_pointer ? ssa::Primitive::ADDR : ssa_param_type;
        ssa::Instruction &alloca_instr = ctx.append_alloca(ssa_slot, alloca_type);
        alloca_instr.set_attr(ssa::Instruction::Attribute::ARG_STORE);

        for (unsigned i = 0; i < pass_method.num_args; i++) {
            ssa::VirtualRegister ssa_arg_reg = ssa_func->next_virtual_reg();

            ssa::Type copy_type =
                i == pass_method.num_args - 1 ? pass_method.last_arg_type : data_layout.get_usize_type();

            ssa::Instruction &loadarg_instr = ctx.append_loadarg(ssa_arg_reg, copy_type, ssa_arg_index);
            loadarg_instr.set_attr(ssa::Instruction::Attribute::SAVE_ARG);

            ssa::Operand store_dst = ssa::Operand::from_register(ssa_slot, ssa::Primitive::ADDR);

            if (i != 0) {
                ssa::VirtualRegister dst_reg = ctx.append_offsetptr(store_dst, 1, data_layout.get_usize_type());
                store_dst = ssa::Operand::from_register(dst_reg, ssa::Primitive::ADDR);
            }

            ssa::Instruction &store_instr =
                ctx.append_store(ssa::Operand::from_register(ssa_arg_reg, copy_type), store_dst);
            store_instr.set_attr(ssa::Instruction::Attribute::SAVE_ARG);

            ssa_arg_index += 1;
        }

        ctx.ssa_param_slots.insert({&sir_param, ssa_slot});
    }

    if (return_method != ReturnMethod::NO_RETURN_VALUE) {
        ctx.get_func_context().ssa_return_slot = ctx.append_alloca(ssa_return_type);
    }

    BlockSSAGenerator(ctx).generate_block(sir_func.block);

    if (!ctx.get_ssa_block()->is_branching()) {
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
        ssa::Value ssa_copy_dst = ctx.append_load(ssa::Primitive::ADDR, ctx.get_func_context().ssa_return_arg_slot);
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

void SSAGenerator::generate_proto_def(const sir::ProtoDef &sir_proto_def) {
    generate_decls(sir_proto_def.block);
}

void SSAGenerator::generate_var_decl(const sir::VarDecl &sir_var_decl) {
    ssa::Global *ssa_global = ssa_mod.get_globals()[ctx.ssa_globals[&sir_var_decl]];

    ssa_global->type = TypeSSAGenerator(ctx).generate(sir_var_decl.type);

    if (sir_var_decl.value) {
        ssa_global->initial_value = GlobalSSAGenerator(ctx).generate_value(sir_var_decl.value);
    }
}

void SSAGenerator::generate_native_var_decl(const sir::NativeVarDecl &sir_native_var_decl) {
    ssa::GlobalDecl *ssa_global = ssa_mod.get_external_globals()[ctx.ssa_extern_globals[&sir_native_var_decl]];
    ssa_global->type = TypeSSAGenerator(ctx).generate(sir_native_var_decl.type);
}

} // namespace lang

} // namespace banjo
