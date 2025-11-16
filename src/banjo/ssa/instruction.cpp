#include "instruction.hpp"

#include "banjo/ssa/primitive.hpp"
#include "banjo/ssa/structure.hpp"

namespace banjo::ssa {

Type Instruction::get_type() const {
    switch (opcode) {
        case Opcode::STORE:
        case Opcode::JMP:
        case Opcode::CJMP:
        case Opcode::FCJMP:
        case Opcode::RET:
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
        case Opcode::SHL:
        case Opcode::SHR:
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
    }
}

} // namespace banjo::ssa
