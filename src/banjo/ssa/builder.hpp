#ifndef BANJO_SSA_BUILDER_H
#define BANJO_SSA_BUILDER_H

#include "banjo/ssa/basic_block.hpp"
#include "banjo/ssa/function.hpp"

#include <vector>

namespace banjo::ssa {

class Builder {

private:
    ssa::Function &func;
    ssa::BasicBlock &block;
    ssa::InstrIter cursor = nullptr;

public:
    Builder(ssa::Function &func, ssa::BasicBlock &block);
    void set_cursor(ssa::InstrIter cursor) { this->cursor = cursor; }

    ssa::Operand emit_load(ssa::Type type, ssa::Operand addr);
    void emit_store(ssa::Operand value, ssa::Operand addr);

    ssa::Operand emit_or(ssa::Operand lhs, ssa::Operand rhs);
    ssa::Operand emit_lshl(ssa::Operand lhs, unsigned rhs);
    ssa::Operand emit_lshl(ssa::Operand lhs, ssa::Operand rhs);
    ssa::Operand emit_lshr(ssa::Operand lhs, unsigned rhs);
    ssa::Operand emit_lshr(ssa::Operand lhs, ssa::Operand rhs);

    ssa::Operand emit_uextend(ssa::Operand value, ssa::Type type);
    ssa::Operand emit_truncate(ssa::Operand value, ssa::Type type);

    ssa::Operand emit_offsetptr(ssa::Operand base, unsigned offset, ssa::Type type);
    ssa::Operand emit_offsetptr(ssa::Operand base, ssa::Operand offset, ssa::Type type);

private:
    ssa::Operand emit_with_dst(ssa::Opcode opcode, std::vector<ssa::Operand> operands, ssa::Type result_type);
    void emit(ssa::Instruction instr);
};

} // namespace banjo::ssa

#endif
