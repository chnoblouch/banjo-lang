#include "ssa_generator_context.hpp"

#include "banjo/ir_builder/ir_builder_utils.hpp"
#include "banjo/sir/sir.hpp"
#include "banjo/ssa_gen/type_ssa_generator.hpp"

namespace banjo {

namespace IRBuilderUtils = ir_builder::IRBuilderUtils;

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
        .arg_regs = {},
    });
}

ir::BasicBlockIter SSAGeneratorContext::create_block(std::string label) {
    return get_ssa_func()->create_block(std::move(label));
}

ir::BasicBlockIter SSAGeneratorContext::create_block() {
    return create_block("b." + std::to_string(next_block_id()));
}

void SSAGeneratorContext::append_block(ir::BasicBlockIter block) {
    get_ssa_func()->append_block(block);
    get_func_context().ssa_block = block;
}

ir::Instruction &SSAGeneratorContext::append_alloca(ir::VirtualRegister dest, ir::Type type) {
    ir::BasicBlockIter block_iter = get_ssa_func()->get_entry_block_iter();
    ir::Instruction instr = ir::Instruction(ir::Opcode::ALLOCA, dest, {ir::Operand::from_type(type)});
    get_func_context().ssa_last_alloca = block_iter->insert_after(get_func_context().ssa_last_alloca, instr);
    return *get_func_context().ssa_last_alloca;
}

ir::VirtualRegister SSAGeneratorContext::append_alloca(ir::Type type) {
    ir::VirtualRegister reg = next_vreg();
    append_alloca(reg, type);
    return reg;
}

ir::Instruction &SSAGeneratorContext::append_store(ir::Operand src, ir::Operand dst) {
    return *get_ssa_block()->append(ir::Instruction(ir::Opcode::STORE, {std::move(src), std::move(dst)}));
}

ir::Value SSAGeneratorContext::append_load(ir::Type type, ir::Operand src) {
    ir::VirtualRegister reg = next_vreg();
    ir::Operand type_operand = ir::Operand::from_type(type);
    get_ssa_block()->append(ir::Instruction(ir::Opcode::LOAD, reg, {type_operand, std::move(src)}));
    return ir::Value::from_register(reg, type);
}

ir::Value SSAGeneratorContext::append_load(ir::Type type, ir::VirtualRegister src) {
    return append_load(type, ssa::Value::from_register(src, ssa::Primitive::ADDR));
}

ir::Instruction &SSAGeneratorContext::append_loadarg(ir::VirtualRegister dst, ir::Type type, unsigned index) {
    ir::Operand type_operand = ir::Operand::from_type(type);
    ir::Operand index_operand = ir::Operand::from_int_immediate(index);
    return *get_ssa_block()->append(ir::Instruction(ir::Opcode::LOADARG, dst, {type_operand, index_operand}));
}

ir::VirtualRegister SSAGeneratorContext::append_loadarg(ir::Type type, unsigned index) {
    ir::VirtualRegister reg = next_vreg();
    append_loadarg(reg, type, index);
    return reg;
}

void SSAGeneratorContext::append_jmp(ir::BasicBlockIter block_iter) {
    if (IRBuilderUtils::is_branching(*get_ssa_block())) {
        return;
    }

    ir::BranchTarget target{.block = block_iter, .args = {}};
    get_ssa_block()->append(ir::Instruction(ir::Opcode::JMP, {ir::Operand::from_branch_target(target)}));
}

void SSAGeneratorContext::append_cjmp(
    ir::Operand lhs,
    ir::Comparison comparison,
    ir::Operand rhs,
    ir::BasicBlockIter true_block_iter,
    ir::BasicBlockIter false_block_iter
) {
    if (get_ssa_block()->get_instrs().get_size() != 0 &&
        IRBuilderUtils::is_branch(get_ssa_block()->get_instrs().get_last())) {
        return;
    }

    std::vector<ir::Operand> operands{
        std::move(lhs),
        ir::Operand::from_comparison(comparison),
        std::move(rhs),
        ir::Operand::from_branch_target({.block = true_block_iter, .args = {}}),
        ir::Operand::from_branch_target({.block = false_block_iter, .args = {}}),
    };

    get_ssa_block()->append(ir::Instruction(ir::Opcode::CJMP, operands));
}

void SSAGeneratorContext::append_fcjmp(
    ir::Operand lhs,
    ir::Comparison comparison,
    ir::Operand rhs,
    ir::BasicBlockIter true_block_iter,
    ir::BasicBlockIter false_block_iter
) {
    if (get_ssa_block()->get_instrs().get_size() != 0 &&
        IRBuilderUtils::is_branch(get_ssa_block()->get_instrs().get_last())) {
        return;
    }

    std::vector<ir::Operand> operands{
        std::move(lhs),
        ir::Operand::from_comparison(comparison),
        std::move(rhs),
        ir::Operand::from_branch_target({.block = true_block_iter, .args = {}}),
        ir::Operand::from_branch_target({.block = false_block_iter, .args = {}}),
    };

    get_ssa_block()->append(ir::Instruction(ir::Opcode::FCJMP, operands));
}

ir::VirtualRegister SSAGeneratorContext::append_offsetptr(ir::Operand base, unsigned offset, ir::Type type) {
    ir::Value offset_val = ir::Value::from_int_immediate(offset, ir::Primitive::I64);
    return append_offsetptr(std::move(base), offset_val, type);
}

ir::VirtualRegister SSAGeneratorContext::append_offsetptr(ir::Operand base, ir::Operand offset, ir::Type type) {
    // FIXME: make sure offset is always an i64

    ir::VirtualRegister dst = next_vreg();
    ir::Value type_val = ir::Value::from_type(type);
    get_ssa_block()->append(ir::Instruction(ir::Opcode::OFFSETPTR, dst, {std::move(base), std::move(offset), type_val})
    );
    return dst;
}

void SSAGeneratorContext::append_memberptr(ir::VirtualRegister dst, ir::Type type, ir::Operand base, unsigned member) {
    return append_memberptr(dst, type, std::move(base), ir::Operand::from_int_immediate(member));
}

ir::VirtualRegister SSAGeneratorContext::append_memberptr(ir::Type type, ir::Operand base, unsigned member) {
    return append_memberptr(type, std::move(base), ir::Operand::from_int_immediate(member));
}

void SSAGeneratorContext::append_memberptr(
    ir::VirtualRegister dst,
    ir::Type type,
    ir::Operand base,
    ir::Operand member
) {
    ir::Value type_val = ir::Value::from_type(type);
    get_ssa_block()->append(ir::Instruction(ir::Opcode::MEMBERPTR, dst, {type_val, std::move(base), std::move(member)})
    );
}

ir::VirtualRegister SSAGeneratorContext::append_memberptr(ir::Type type, ir::Operand base, ir::Operand member) {
    ir::VirtualRegister reg = next_vreg();
    append_memberptr(reg, type, std::move(base), std::move(member));
    return reg;
}

void SSAGeneratorContext::append_ret(ir::Operand val) {
    get_ssa_block()->append(ir::Instruction(ir::Opcode::RET, {std::move(val)}));
}

void SSAGeneratorContext::append_ret() {
    get_ssa_block()->append(ir::Instruction(ir::Opcode::RET));
}

void SSAGeneratorContext::append_copy(ir::Operand dst, ir::Operand src, ir::Type type) {
    ir::Value type_val = ir::Operand::from_type(type);
    get_ssa_block()->append(ir::Instruction(ir::Opcode::COPY, {std::move(dst), std::move(src), type_val}));
}

ReturnMethod SSAGeneratorContext::get_return_method(const ssa::Type return_type) {
    unsigned size = target->get_data_layout().get_size(return_type);
    if (size == 0) {
        return ReturnMethod::NO_RETURN_VALUE;
    }

    bool fits_in_reg = target->get_data_layout().fits_in_register(return_type);
    return fits_in_reg ? ReturnMethod::IN_REGISTER : ReturnMethod::VIA_POINTER_ARG;
}

ir::Structure *SSAGeneratorContext::get_tuple_struct(const std::vector<ir::Type> &member_types) {
    for (ir::Structure *struct_ : tuple_structs) {
        if (struct_->get_members().size() != member_types.size()) {
            continue;
        }

        bool compatible = true;

        for (unsigned i = 0; i < member_types.size(); i++) {
            if (struct_->get_members()[i].type != member_types[i]) {
                compatible = false;
                break;
            }
        }

        if (compatible) {
            return struct_;
        }
    }

    ir::Structure *struct_ = new ir::Structure("tuple." + std::to_string(tuple_structs.size()));
    for (unsigned i = 0; i < member_types.size(); i++) {
        struct_->add({std::to_string(i), member_types[i]});
    }

    ssa_mod->add(struct_);
    tuple_structs.push_back(struct_);

    return struct_;
}

} // namespace lang

} // namespace banjo
