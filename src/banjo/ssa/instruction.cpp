#include "instruction.hpp"

#include "banjo/ssa/primitive.hpp"
#include "banjo/ssa/structure.hpp"

namespace banjo::ssa {

bool Instruction::might_access_memory() const {
    switch (opcode) {
        case Opcode::LOAD:
        case Opcode::STORE:
        case Opcode::CALL:
        case Opcode::COPY:
        case Opcode::ATOMIC_LOAD:
        case Opcode::ATOMIC_STORE:
        case Opcode::ATOMIC_ADD:
        case Opcode::ATOMIC_SUB:
        case Opcode::ATOMIC_AND:
        case Opcode::ATOMIC_OR:
        case Opcode::ATOMIC_XOR: return true;

        case Opcode::ALLOCA:
        case Opcode::LOADARG:
        case Opcode::ADD:
        case Opcode::SUB:
        case Opcode::MUL:
        case Opcode::SDIV:
        case Opcode::SREM:
        case Opcode::UDIV:
        case Opcode::UREM:
        case Opcode::FADD:
        case Opcode::FSUB:
        case Opcode::FMUL:
        case Opcode::FDIV:
        case Opcode::AND:
        case Opcode::OR:
        case Opcode::XOR:
        case Opcode::LSHL:
        case Opcode::LSHR:
        case Opcode::ASHR:
        case Opcode::JMP:
        case Opcode::CJMP:
        case Opcode::FCJMP:
        case Opcode::SELECT:
        case Opcode::RET:
        case Opcode::UEXTEND:
        case Opcode::SEXTEND:
        case Opcode::TRUNCATE:
        case Opcode::FPROMOTE:
        case Opcode::FDEMOTE:
        case Opcode::UTOF:
        case Opcode::STOF:
        case Opcode::FTOU:
        case Opcode::FTOS:
        case Opcode::MEMBERPTR:
        case Opcode::OFFSETPTR:
        case Opcode::SQRT:
        case Opcode::FRAME_ADDRESS: return false;
    }
}

bool Instruction::is_branching() const {
    return opcode == ssa::Opcode::JMP || opcode == ssa::Opcode::CJMP || opcode == ssa::Opcode::FCJMP;
}

bool Instruction::is_cond_branch() const {
    return opcode == ssa::Opcode::CJMP || opcode == ssa::Opcode::FCJMP;
}

Type Instruction::get_type() const {
    switch (opcode) {
        case Opcode::STORE:
        case Opcode::JMP:
        case Opcode::CJMP:
        case Opcode::FCJMP:
        case Opcode::RET:
        case Opcode::ATOMIC_STORE:
        case Opcode::COPY: return ssa::Primitive::VOID;
        case Opcode::OFFSETPTR: return ssa::Primitive::ADDR;
        case Opcode::ALLOCA:
        case Opcode::LOAD:
        case Opcode::LOADARG:
        case Opcode::ADD:
        case Opcode::SUB:
        case Opcode::MUL:
        case Opcode::SDIV:
        case Opcode::SREM:
        case Opcode::UDIV:
        case Opcode::UREM:
        case Opcode::FADD:
        case Opcode::FSUB:
        case Opcode::FMUL:
        case Opcode::FDIV:
        case Opcode::AND:
        case Opcode::OR:
        case Opcode::XOR:
        case Opcode::LSHL:
        case Opcode::LSHR:
        case Opcode::ASHR:
        case Opcode::ATOMIC_LOAD:
        case Opcode::ATOMIC_ADD:
        case Opcode::ATOMIC_SUB:
        case Opcode::ATOMIC_AND:
        case Opcode::ATOMIC_OR:
        case Opcode::ATOMIC_XOR:
        case Opcode::SQRT: return operands[0].get_type();
        case Opcode::UEXTEND: return operands[1].get_type();
        case Opcode::SEXTEND: return operands[1].get_type();
        case Opcode::TRUNCATE: return operands[1].get_type();
        case Opcode::FPROMOTE: return operands[1].get_type();
        case Opcode::FDEMOTE: return operands[1].get_type();
        case Opcode::UTOF: return operands[1].get_type();
        case Opcode::STOF: return operands[1].get_type();
        case Opcode::FTOU: return operands[1].get_type();
        case Opcode::FTOS: return operands[1].get_type();
        case Opcode::SELECT: return operands[3].get_type();
        case Opcode::CALL: return dest ? operands[0].get_type() : ssa::Primitive::VOID;
        case Opcode::MEMBERPTR: {
            ssa::Structure &struct_ = *operands[0].get_type().get_struct();
            unsigned member_index = operands[2].get_int_immediate().to_unsigned();
            return struct_.members[member_index].type;
        }
        case Opcode::FRAME_ADDRESS: return ssa::Primitive::ADDR;
    }
}

} // namespace banjo::ssa
