#include "ssa_generator_context.hpp"

#include "banjo/sir/sir.hpp"
#include "banjo/ssa_gen/type_ssa_generator.hpp"
#include <algorithm>

namespace banjo {

namespace lang {

SSAGeneratorContext::SSAGeneratorContext(target::Target *target) : target(target) {}

SSAGeneratorContext::DeclContext &SSAGeneratorContext::push_decl_context() {
    decl_contexts.push_front({});
    return decl_contexts.front();
}

void SSAGeneratorContext::push_func_context(ssa::Function *ssa_func) {
    func_contexts.push({
        .ssa_func = ssa_func,
        .ssa_block = ssa_func->get_entry_block_iter(),
        .ssa_last_alloca = ssa_func->get_entry_block().get_instrs().get_header(),
    });
}

ssa::BasicBlockIter SSAGeneratorContext::create_block(std::string label) {
    return get_ssa_func()->create_block(std::move(label));
}

ssa::BasicBlockIter SSAGeneratorContext::create_block() {
    return create_block("b." + std::to_string(next_block_id()));
}

void SSAGeneratorContext::append_block(ssa::BasicBlockIter block) {
    get_ssa_func()->append_block(block);
    get_func_context().ssa_block = block;
}

ssa::Instruction &SSAGeneratorContext::append_alloca(ssa::VirtualRegister dest, ssa::Type type) {
    ssa::BasicBlockIter block_iter = get_ssa_func()->get_entry_block_iter();
    ssa::Instruction instr = ssa::Instruction(ssa::Opcode::ALLOCA, dest, {ssa::Operand::from_type(type)});
    get_func_context().ssa_last_alloca = block_iter->insert_after(get_func_context().ssa_last_alloca, instr);
    return *get_func_context().ssa_last_alloca;
}

ssa::VirtualRegister SSAGeneratorContext::append_alloca(ssa::Type type) {
    ssa::VirtualRegister reg = next_vreg();
    append_alloca(reg, type);
    return reg;
}

ssa::Instruction &SSAGeneratorContext::append_store(ssa::Operand src, ssa::Operand dst) {
    return *get_ssa_block()->append(ssa::Instruction(ssa::Opcode::STORE, {std::move(src), std::move(dst)}));
}

ssa::Instruction &SSAGeneratorContext::append_store(ssa::Operand src, ssa::VirtualRegister dst) {
    return append_store(std::move(src), ssa::Operand::from_register(dst, ssa::Primitive::ADDR));
}

ssa::Value SSAGeneratorContext::append_load(ssa::Type type, ssa::Operand src) {
    ssa::VirtualRegister reg = next_vreg();
    ssa::Operand type_operand = ssa::Operand::from_type(type);
    get_ssa_block()->append(ssa::Instruction(ssa::Opcode::LOAD, reg, {type_operand, std::move(src)}));
    return ssa::Value::from_register(reg, type);
}

ssa::Value SSAGeneratorContext::append_load(ssa::Type type, ssa::VirtualRegister src) {
    return append_load(type, ssa::Value::from_register(src, ssa::Primitive::ADDR));
}

ssa::Instruction &SSAGeneratorContext::append_loadarg(ssa::VirtualRegister dst, ssa::Type type, unsigned index) {
    ssa::Operand type_operand = ssa::Operand::from_type(type);
    ssa::Operand index_operand = ssa::Operand::from_int_immediate(index);
    return *get_ssa_block()->append(ssa::Instruction(ssa::Opcode::LOADARG, dst, {type_operand, index_operand}));
}

ssa::VirtualRegister SSAGeneratorContext::append_loadarg(ssa::Type type, unsigned index) {
    ssa::VirtualRegister reg = next_vreg();
    append_loadarg(reg, type, index);
    return reg;
}

void SSAGeneratorContext::append_jmp(ssa::BasicBlockIter block_iter) {
    if (get_ssa_block()->is_branching()) {
        return;
    }

    ssa::BranchTarget target{.block = block_iter, .args = {}};
    get_ssa_block()->append(ssa::Instruction(ssa::Opcode::JMP, {ssa::Operand::from_branch_target(target)}));
}

void SSAGeneratorContext::append_cjmp(
    ssa::Operand lhs,
    ssa::Comparison comparison,
    ssa::Operand rhs,
    ssa::BasicBlockIter true_block_iter,
    ssa::BasicBlockIter false_block_iter
) {
    if (get_ssa_block()->is_branching()) {
        return;
    }

    std::vector<ssa::Operand> operands{
        std::move(lhs),
        ssa::Operand::from_comparison(comparison),
        std::move(rhs),
        ssa::Operand::from_branch_target({.block = true_block_iter, .args = {}}),
        ssa::Operand::from_branch_target({.block = false_block_iter, .args = {}}),
    };

    get_ssa_block()->append(ssa::Instruction(ssa::Opcode::CJMP, operands));
}

void SSAGeneratorContext::append_fcjmp(
    ssa::Operand lhs,
    ssa::Comparison comparison,
    ssa::Operand rhs,
    ssa::BasicBlockIter true_block_iter,
    ssa::BasicBlockIter false_block_iter
) {
    if (get_ssa_block()->is_branching()) {
        return;
    }

    std::vector<ssa::Operand> operands{
        std::move(lhs),
        ssa::Operand::from_comparison(comparison),
        std::move(rhs),
        ssa::Operand::from_branch_target({.block = true_block_iter, .args = {}}),
        ssa::Operand::from_branch_target({.block = false_block_iter, .args = {}}),
    };

    get_ssa_block()->append(ssa::Instruction(ssa::Opcode::FCJMP, operands));
}

ssa::VirtualRegister SSAGeneratorContext::append_offsetptr(ssa::Operand base, unsigned offset, ssa::Type type) {
    ssa::Type usize_type = target->get_data_layout().get_usize_type();
    ssa::Value offset_val = ssa::Value::from_int_immediate(offset, usize_type);
    return append_offsetptr(std::move(base), offset_val, type);
}

ssa::VirtualRegister SSAGeneratorContext::append_offsetptr(ssa::Operand base, ssa::Operand offset, ssa::Type type) {
    ssa::VirtualRegister dst = next_vreg();
    ssa::Value type_operand = ssa::Value::from_type(type);

    get_ssa_block()->append(
        ssa::Instruction(ssa::Opcode::OFFSETPTR, dst, {std::move(base), std::move(offset), type_operand})
    );

    return dst;
}

void SSAGeneratorContext::append_memberptr(
    ssa::VirtualRegister dst,
    ssa::Type type,
    ssa::Operand base,
    unsigned member
) {
    std::vector<ssa::Operand> operands{
        ssa::Value::from_type(type),
        std::move(base),
        ssa::Operand::from_int_immediate(member),
    };

    get_ssa_block()->append(ssa::Instruction(ssa::Opcode::MEMBERPTR, dst, operands));
}

ssa::VirtualRegister SSAGeneratorContext::append_memberptr(ssa::Type type, ssa::Operand base, unsigned member) {
    ssa::VirtualRegister reg = next_vreg();
    append_memberptr(reg, type, std::move(base), member);
    return reg;
}

ssa::Value SSAGeneratorContext::append_memberptr_val(ssa::Type type, ssa::Operand base, unsigned member) {
    ssa::VirtualRegister reg = append_memberptr(type, std::move(base), member);
    return ssa::Value::from_register(reg, ssa::Primitive::ADDR);
}

void SSAGeneratorContext::append_ret(ssa::Operand val) {
    get_ssa_block()->append(ssa::Instruction(ssa::Opcode::RET, {std::move(val)}));
}

void SSAGeneratorContext::append_ret() {
    get_ssa_block()->append(ssa::Instruction(ssa::Opcode::RET));
}

void SSAGeneratorContext::append_copy(ssa::Operand dst, ssa::Operand src, ssa::Type type) {
    ssa::Value type_val = ssa::Operand::from_type(type);
    get_ssa_block()->append(ssa::Instruction(ssa::Opcode::COPY, {std::move(dst), std::move(src), type_val}));
}

ReturnMethod SSAGeneratorContext::get_return_method(const ssa::Type return_type) {
    if (return_type.is_primitive(ssa::Primitive::VOID)) {
        return ReturnMethod::NO_RETURN_VALUE;
    }

    bool fits_in_reg = target->get_data_layout().fits_in_register(return_type);
    return fits_in_reg ? ReturnMethod::IN_REGISTER : ReturnMethod::VIA_POINTER_ARG;
}

ssa::Structure *SSAGeneratorContext::create_struct(const sir::StructDef &sir_struct_def) {
    auto iter = ssa_structs.find(&sir_struct_def);
    if (iter != ssa_structs.end()) {
        return iter->second;
    }

    ssa::Structure *ssa_struct = new ssa::Structure(std::string{sir_struct_def.ident.value});
    ssa_mod->add(ssa_struct);

    if (sir_struct_def.get_layout() == sir::Attributes::Layout::DEFAULT) {
        for (sir::StructField *sir_field : sir_struct_def.fields) {
            ssa_struct->add({
                .name = std::string{sir_field->ident.value},
                .type = TypeSSAGenerator(*this).generate(sir_field->type),
            });
        }
    } else if (sir_struct_def.get_layout() == sir::Attributes::Layout::OVERLAPPING) {
        ssa::Type largest_type = ssa::Primitive::VOID;
        unsigned largest_size = 0;

        for (sir::StructField *sir_field : sir_struct_def.fields) {
            ssa::Type ssa_type = TypeSSAGenerator(*this).generate(sir_field->type);
            unsigned size = target->get_data_layout().get_size(ssa_type);

            if (size > largest_size) {
                largest_type = ssa_type;
                largest_size = size;
            }
        }

        ssa_struct->add({
            .name = "data",
            .type = largest_type,
        });
    }

    ssa_structs.insert({&sir_struct_def, ssa_struct});
    return ssa_struct;
}

ssa::Structure *SSAGeneratorContext::create_union(const sir::UnionDef &sir_union_def) {
    auto iter = ssa_structs.find(&sir_union_def);
    if (iter != ssa_structs.end()) {
        return iter->second;
    }

    ssa::Structure *ssa_struct = new ssa::Structure(std::string{sir_union_def.ident.value});
    ssa_mod->add(ssa_struct);

    ssa_struct->add({
        .name = "tag",
        .type = ssa::Primitive::U32,
    });

    ssa::Type largest_case_type = ssa::Primitive::VOID;
    unsigned largest_case_size = 0;

    for (sir::UnionCase *sir_union_case : sir_union_def.cases) {
        ssa::Structure *case_ssa_struct = create_union_case(*sir_union_case);
        unsigned case_size = target->get_data_layout().get_size(case_ssa_struct);

        if (case_size > largest_case_size) {
            largest_case_type = case_ssa_struct;
            largest_case_size = case_size;
        }
    }

    ssa_struct->add({
        .name = "data",
        .type = largest_case_type,
    });

    ssa_structs.insert({&sir_union_def, ssa_struct});
    return ssa_struct;
}

ssa::Structure *SSAGeneratorContext::create_union_case(const sir::UnionCase &sir_union_case) {
    auto iter = ssa_structs.find(&sir_union_case);
    if (iter != ssa_structs.end()) {
        return iter->second;
    }

    ssa::Structure *ssa_struct = new ssa::Structure(std::string{sir_union_case.ident.value});
    ssa_mod->add(ssa_struct);

    for (unsigned i = 0; i < sir_union_case.fields.size(); i++) {
        ssa_struct->add({
            .name = std::to_string(i),
            .type = TypeSSAGenerator(*this).generate(sir_union_case.fields[i].type),
        });
    }

    ssa_structs.insert({&sir_union_case, ssa_struct});
    return ssa_struct;
}

ssa::Structure *SSAGeneratorContext::get_tuple_struct(const std::vector<ssa::Type> &member_types) {
    for (ssa::Structure *ssa_struct : tuple_structs) {
        if (ssa_struct->members.size() != member_types.size()) {
            continue;
        }

        bool compatible = true;

        for (unsigned i = 0; i < member_types.size(); i++) {
            if (ssa_struct->members[i].type != member_types[i]) {
                compatible = false;
                break;
            }
        }

        if (compatible) {
            return ssa_struct;
        }
    }

    ssa::Structure *ssa_struct = new ssa::Structure("tuple." + std::to_string(tuple_structs.size()));

    for (unsigned i = 0; i < member_types.size(); i++) {
        ssa_struct->add({
            .name = std::to_string(i),
            .type = member_types[i],
        });
    }

    ssa_mod->add(ssa_struct);
    tuple_structs.push_back(ssa_struct);
    return ssa_struct;
}

const std::vector<unsigned> &SSAGeneratorContext::create_vtables(const sir::StructDef &struct_def) {
    auto iter = ssa_vtables.find(&struct_def);
    if (iter != ssa_vtables.end()) {
        return iter->second;
    }

    const sir::SymbolTable &symbol_table = *struct_def.block.symbol_table;

    for (unsigned i = 0; i < struct_def.impls.size(); i++) {
        const sir::Expr &impl = struct_def.impls[i];
        const sir::ProtoDef &proto_def = impl.as_symbol<sir::ProtoDef>();

        std::string vtable_name = "vtable." + std::string{struct_def.ident.value} + "." + std::to_string(i);

        for (unsigned j = 0; j < proto_def.func_decls.size(); j++) {
            sir::ProtoFuncDecl func_decl = proto_def.func_decls[j];
            const sir::FuncDef &func_def = symbol_table.symbols.at(func_decl.get_ident().value).as<sir::FuncDef>();

            std::string vtable_entry_name = vtable_name;
            if (j != 0) {
                vtable_entry_name += "." + std::to_string(j);
            }

            ssa::Function *ssa_func = ssa_funcs.at(&func_def);

            ssa_mod->add(new ssa::Global{
                .name = vtable_entry_name,
                .type = ssa::Primitive::ADDR,
                .initial_value = ssa_func,
                .external = false,
            });

            if (j == 0) {
                ssa_vtables[&struct_def].push_back(ssa_mod->get_globals().size() - 1);
            }
        }
    }

    return ssa_vtables[&struct_def];
}

ssa::Type SSAGeneratorContext::get_fat_pointer_type() {
    return get_tuple_struct({ssa::Primitive::ADDR, ssa::Primitive::ADDR});
}

} // namespace lang

} // namespace banjo
