#include "ir_builder_context.hpp"

#include <utility>

#include "ir_builder/ir_builder_utils.hpp"

namespace ir_builder {

IRBuilderContext::IRBuilderContext(target::Target *target) : target(target) {
    scopes.push({});
}

void IRBuilderContext::set_current_func(ir::Function *current_func) {
    this->current_func = current_func;

    if (current_func) {
        has_branch = false;
        cur_last_alloca = current_func->get_entry_block().get_instrs().get_header();
    }
}

Scope &IRBuilderContext::push_scope() {
    scopes.push(scopes.top());
    return scopes.top();
}

void IRBuilderContext::pop_scope() {
    scopes.pop();
}

std::string IRBuilderContext::next_string_name() {
    return "str." + std::to_string(string_name_id++);
}

int IRBuilderContext::next_block_id() {
    return block_id++;
}

int IRBuilderContext::next_if_chain_id() {
    return if_chain_id++;
}

int IRBuilderContext::next_switch_id() {
    return switch_id++;
}

int IRBuilderContext::next_while_id() {
    return while_id++;
}

int IRBuilderContext::next_for_id() {
    return for_id++;
}

int IRBuilderContext::next_or_id() {
    return or_id++;
}

int IRBuilderContext::next_and_id() {
    return and_id++;
}

int IRBuilderContext::next_not_id() {
    return not_id++;
}

int IRBuilderContext::next_cmp_to_val_id() {
    return cmp_to_val_id++;
}

int IRBuilderContext::next_deinit_flag_id() {
    return deinit_flag_id++;
}

unsigned IRBuilderContext::get_size(const ir::Type &type) {
    return target->get_data_layout().get_size(type, *current_mod);
}

ir::BasicBlockIter IRBuilderContext::create_block(std::string label) {
    return current_func->create_block(std::move(label));
}

void IRBuilderContext::append_block(ir::BasicBlockIter block) {
    current_func->append_block(block);
    cur_block_iter = block;
}

ir::Instruction &IRBuilderContext::append_alloca(ir::VirtualRegister dest, ir::Type type) {
    ir::BasicBlockIter block_iter = current_func->get_entry_block_iter();
    ir::Instruction instr = ir::Instruction(ir::Opcode::ALLOCA, dest, {ir::Operand::from_type(std::move(type))});
    cur_last_alloca = block_iter->insert_after(cur_last_alloca, instr);
    return *cur_last_alloca;
}

ir::VirtualRegister IRBuilderContext::append_alloca(ir::Type type) {
    ir::VirtualRegister reg = current_func->next_virtual_reg();
    append_alloca(reg, std::move(type));
    return reg;
}

ir::Instruction &IRBuilderContext::append_store(ir::Operand src, ir::Operand dst) {
    return *cur_block_iter->append(ir::Instruction(ir::Opcode::STORE, {std::move(src), std::move(dst)}));
}

void IRBuilderContext::append_load(ir::VirtualRegister dest, ir::Operand src) {
    cur_block_iter->append(ir::Instruction(
        ir::Opcode::LOAD,
        dest,
        {
            ir::Operand::from_type(src.get_type().deref()),
            std::move(src),
        }
    ));
}

ir::VirtualRegister IRBuilderContext::append_load(ir::Operand src) {
    ir::VirtualRegister reg = current_func->next_virtual_reg();
    append_load(reg, std::move(src));
    return reg;
}

ir::Instruction &IRBuilderContext::append_loadarg(ir::VirtualRegister dest, ir::Operand src) {
    return *cur_block_iter->append(ir::Instruction(
        ir::Opcode::LOADARG,
        dest,
        {
            ir::Operand::from_type(src.get_type().deref()),
            std::move(src),
        }
    ));
}

ir::VirtualRegister IRBuilderContext::append_loadarg(ir::Operand src) {
    ir::VirtualRegister reg = current_func->next_virtual_reg();
    append_loadarg(reg, std::move(src));
    return reg;
}

void IRBuilderContext::append_jmp(ir::BasicBlockIter block_iter) {
    if (IRBuilderUtils::is_branching(*cur_block_iter)) {
        return;
    }

    ir::BranchTarget target{.block = block_iter, .args = {}};
    cur_block_iter->append(ir::Instruction(ir::Opcode::JMP, {ir::Operand::from_branch_target(target)}));
}

void IRBuilderContext::append_cjmp(
    ir::Operand left,
    ir::Comparison comparison,
    ir::Operand right,
    ir::BasicBlockIter true_block_iter,
    ir::BasicBlockIter false_block_iter
) {
    if (cur_block_iter->get_instrs().get_size() != 0 &&
        IRBuilderUtils::is_branch(cur_block_iter->get_instrs().get_last())) {
        return;
    }

    cur_block_iter->append(ir::Instruction(
        ir::Opcode::CJMP,
        {
            std::move(left),
            ir::Operand::from_comparison(comparison),
            std::move(right),
            ir::Operand::from_branch_target({.block = true_block_iter, .args = {}}),
            ir::Operand::from_branch_target({.block = false_block_iter, .args = {}}),
        }
    ));
}

void IRBuilderContext::append_offsetptr(ir::VirtualRegister dest, ir::Operand base, ir::Operand offset) {
    // HACK: don't do this here
    offset.set_type(ir::Primitive::I64);
    cur_block_iter->append(ir::Instruction(ir::Opcode::OFFSETPTR, dest, {std::move(base), std::move(offset)}));
}

ir::VirtualRegister IRBuilderContext::append_offsetptr(ir::Operand base, ir::Operand offset) {
    ir::VirtualRegister reg = current_func->next_virtual_reg();
    append_offsetptr(reg, std::move(base), std::move(offset));
    return reg;
}

void IRBuilderContext::append_memberptr(ir::VirtualRegister dest, ir::Operand base, int member) {
    return append_memberptr(dest, std::move(base), ir::Operand::from_int_immediate(member));
}

ir::VirtualRegister IRBuilderContext::append_memberptr(ir::Operand base, int member) {
    return append_memberptr(std::move(base), ir::Operand::from_int_immediate(member));
}

void IRBuilderContext::append_memberptr(ir::VirtualRegister dest, ir::Operand base, ir::Operand member) {
    cur_block_iter->append(ir::Instruction(ir::Opcode::MEMBERPTR, dest, {std::move(base), std::move(member)}));
}

ir::VirtualRegister IRBuilderContext::append_memberptr(ir::Operand base, ir::Operand member) {
    ir::VirtualRegister reg = current_func->next_virtual_reg();
    append_memberptr(reg, std::move(base), std::move(member));
    return reg;
}

void IRBuilderContext::append_ret(ir::Operand val) {
    cur_block_iter->append(ir::Instruction(ir::Opcode::RET, {std::move(val)}));
}

void IRBuilderContext::append_ret() {
    cur_block_iter->append(ir::Instruction(ir::Opcode::RET));
}

void IRBuilderContext::append_copy(ir::Operand dst, ir::Operand src, unsigned size) {
    ir::Value size_operand = ir::Operand::from_int_immediate(size, ir::Primitive::I64);
    cur_block_iter->append(ir::Instruction(ir::Opcode::COPY, {std::move(dst), std::move(src), size_operand}));
}

} // namespace ir_builder
