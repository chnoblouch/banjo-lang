#include "builder.hpp"

#include "banjo/ssa/opcode.hpp"
#include "banjo/ssa/operand.hpp"
#include "banjo/ssa/primitive.hpp"
#include "banjo/ssa/virtual_register.hpp"

#include <utility>

namespace banjo::ssa {

Builder::Builder(ssa::Function &func, ssa::BasicBlock &block) : func{func}, block{block} {}

ssa::Operand Builder::emit_load(ssa::Type type, ssa::Operand addr) {
    ssa::Operand type_operand = ssa::Operand::from_type(type);
    return emit_with_dst(ssa::Opcode::LOAD, {type_operand, std::move(addr)}, type);
}

void Builder::emit_store(ssa::Operand value, ssa::Operand addr) {
    emit({ssa::Opcode::STORE, {std::move(value), std::move(addr)}});
}

ssa::Operand Builder::emit_or(ssa::Operand lhs, ssa::Operand rhs) {
    ASSERT(lhs.get_type() == rhs.get_type());

    ssa::Type type = lhs.get_type();
    return emit_with_dst(ssa::Opcode::OR, {std::move(lhs), std::move(rhs)}, type);
}

ssa::Operand Builder::emit_lshl(ssa::Operand lhs, unsigned rhs) {
    ssa::Operand rhs_operand = ssa::Operand::from_int_immediate(rhs, lhs.get_type());
    return emit_lshl(std::move(lhs), rhs_operand);
}

ssa::Operand Builder::emit_lshl(ssa::Operand lhs, ssa::Operand rhs) {
    ASSERT(lhs.get_type() == rhs.get_type());

    ssa::Type type = lhs.get_type();
    return emit_with_dst(ssa::Opcode::LSHL, {std::move(lhs), std::move(rhs)}, type);
}

ssa::Operand Builder::emit_lshr(ssa::Operand lhs, unsigned rhs) {
    ssa::Operand rhs_operand = ssa::Operand::from_int_immediate(rhs, lhs.get_type());
    return emit_lshr(std::move(lhs), rhs_operand);
}

ssa::Operand Builder::emit_lshr(ssa::Operand lhs, ssa::Operand rhs) {
    ASSERT(lhs.get_type() == rhs.get_type());

    ssa::Type type = lhs.get_type();
    return emit_with_dst(ssa::Opcode::LSHR, {std::move(lhs), std::move(rhs)}, type);
}

ssa::Operand Builder::emit_uextend(ssa::Operand value, ssa::Type type) {
    ssa::Operand type_operand = ssa::Operand::from_type(type);
    return emit_with_dst(ssa::Opcode::UEXTEND, {std::move(value), type_operand}, type);
}

ssa::Operand Builder::emit_truncate(ssa::Operand value, ssa::Type type) {
    ssa::Operand type_operand = ssa::Operand::from_type(type);
    return emit_with_dst(ssa::Opcode::TRUNCATE, {std::move(value), type_operand}, type);
}

ssa::Operand Builder::emit_offsetptr(ssa::Operand base, unsigned offset, ssa::Type type) {
    ssa::Operand offset_operand = ssa::Operand::from_int_immediate(offset);
    return emit_offsetptr(std::move(base), offset_operand, type);
}

ssa::Operand Builder::emit_offsetptr(ssa::Operand base, ssa::Operand offset, ssa::Type type) {
    ssa::Operand type_operand = ssa::Operand::from_type(type);

    return emit_with_dst(
        ssa::Opcode::OFFSETPTR,
        {std::move(base), std::move(offset), type_operand},
        ssa::Primitive::ADDR
    );
}

ssa::Operand Builder::emit_with_dst(ssa::Opcode opcode, std::vector<ssa::Operand> operands, ssa::Type result_type) {
    ssa::VirtualRegister dst = func.next_virtual_reg();
    emit({opcode, dst, std::move(operands)});
    return ssa::Operand::from_register(dst, result_type);
}

void Builder::emit(ssa::Instruction instr) {
    if (cursor) {
        cursor = block.insert_after(cursor, std::move(instr));
    } else {
        block.append(std::move(instr));
    }
}

} // namespace banjo::ssa
