#include "ir_builder_context.hpp"

#include "ir/primitive.hpp"
#include "ir_builder/ir_builder_utils.hpp"

#include <cassert>
#include <utility>

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
    return target->get_data_layout().get_size(type);
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

void IRBuilderContext::append_load(ir::VirtualRegister dest, ir::Type type, ir::Operand src) {
    ir::Operand type_operand = ir::Operand::from_type(std::move(type));
    cur_block_iter->append(ir::Instruction(ir::Opcode::LOAD, dest, {type_operand, std::move(src)}));
}

ir::Value IRBuilderContext::append_load(ir::Type type, ir::Operand src) {
    ir::VirtualRegister reg = current_func->next_virtual_reg();
    append_load(reg, type, std::move(src));
    return ir::Value::from_register(reg, std::move(type));
}

ir::Instruction &IRBuilderContext::append_loadarg(ir::VirtualRegister dst, ir::Type type, unsigned index) {
    ir::Operand type_operand = ir::Operand::from_type(std::move(type));
    ir::Operand index_operand = ir::Operand::from_int_immediate(index);
    return *cur_block_iter->append(ir::Instruction(ir::Opcode::LOADARG, dst, {type_operand, index_operand}));
}

ir::VirtualRegister IRBuilderContext::append_loadarg(ir::Type type, unsigned index) {
    ir::VirtualRegister reg = current_func->next_virtual_reg();
    append_loadarg(reg, std::move(type), index);
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

ir::VirtualRegister IRBuilderContext::append_offsetptr(ir::Operand base, unsigned offset, ir::Type type) {
    ir::Value offset_val = ir::Value::from_int_immediate(offset, ir::Primitive::I64);
    return append_offsetptr(std::move(base), offset_val, std::move(type));
}

ir::VirtualRegister IRBuilderContext::append_offsetptr(ir::Operand base, ir::Operand offset, ir::Type type) {
    // FIXME: make sure offset is always an i64

    ir::VirtualRegister dst = current_func->next_virtual_reg();
    ir::Value type_val = ir::Value::from_type(std::move(type));
    cur_block_iter->append(ir::Instruction(ir::Opcode::OFFSETPTR, dst, {std::move(base), std::move(offset), type_val}));
    return dst;
}

void IRBuilderContext::append_memberptr(ir::VirtualRegister dst, ir::Type type, ir::Operand base, unsigned member) {
    return append_memberptr(dst, std::move(type), std::move(base), ir::Operand::from_int_immediate(member));
}

ir::VirtualRegister IRBuilderContext::append_memberptr(ir::Type type, ir::Operand base, unsigned member) {
    return append_memberptr(std::move(type), std::move(base), ir::Operand::from_int_immediate(member));
}

void IRBuilderContext::append_memberptr(ir::VirtualRegister dst, ir::Type type, ir::Operand base, ir::Operand member) {
    ir::Value type_val = ir::Value::from_type(std::move(type));
    cur_block_iter->append(ir::Instruction(ir::Opcode::MEMBERPTR, dst, {type_val, std::move(base), std::move(member)}));
}

ir::VirtualRegister IRBuilderContext::append_memberptr(ir::Type type, ir::Operand base, ir::Operand member) {
    ir::VirtualRegister reg = current_func->next_virtual_reg();
    append_memberptr(reg, std::move(type), std::move(base), std::move(member));
    return reg;
}

void IRBuilderContext::append_ret(ir::Operand val) {
    cur_block_iter->append(ir::Instruction(ir::Opcode::RET, {std::move(val)}));
}

void IRBuilderContext::append_ret() {
    cur_block_iter->append(ir::Instruction(ir::Opcode::RET));
}

void IRBuilderContext::append_copy(ir::Operand dst, ir::Operand src, ir::Type type) {
    ir::Value type_val = ir::Operand::from_type(std::move(type));
    cur_block_iter->append(ir::Instruction(ir::Opcode::COPY, {std::move(dst), std::move(src), type_val}));
}

} // namespace ir_builder
