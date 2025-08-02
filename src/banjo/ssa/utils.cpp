#include "utils.hpp"

namespace banjo::ssa {

Type get_result_type(Instruction &instr, Type addr_type) {
    switch (instr.get_opcode()) {
        case Opcode::ALLOCA: return addr_type;
        case Opcode::LOAD: return instr.get_operand(0).get_type();
        case Opcode::STORE: return Primitive::VOID;
        case Opcode::LOADARG: return instr.get_operand(0).get_type();
        case Opcode::ADD: return instr.get_operand(0).get_type();
        case Opcode::SUB: return instr.get_operand(0).get_type();
        case Opcode::MUL: return instr.get_operand(0).get_type();
        case Opcode::SDIV: return instr.get_operand(0).get_type();
        case Opcode::SREM: return instr.get_operand(0).get_type();
        case Opcode::UDIV: return instr.get_operand(0).get_type();
        case Opcode::UREM: return instr.get_operand(0).get_type();
        case Opcode::FADD: return instr.get_operand(0).get_type();
        case Opcode::FSUB: return instr.get_operand(0).get_type();
        case Opcode::FMUL: return instr.get_operand(0).get_type();
        case Opcode::FDIV: return instr.get_operand(0).get_type();
        case Opcode::AND: return instr.get_operand(0).get_type();
        case Opcode::OR: return instr.get_operand(0).get_type();
        case Opcode::XOR: return instr.get_operand(0).get_type();
        case Opcode::SHL: return instr.get_operand(0).get_type();
        case Opcode::SHR: return instr.get_operand(0).get_type();
        case Opcode::JMP: return Primitive::VOID;
        case Opcode::CJMP: return Primitive::VOID;
        case Opcode::FCJMP: return Primitive::VOID;
        case Opcode::SELECT: return instr.get_operand(3).get_type();
        case Opcode::CALL: return instr.get_operand(0).get_type();
        case Opcode::RET: return Primitive::VOID;
        case Opcode::UEXTEND: return instr.get_operand(1).get_type();
        case Opcode::SEXTEND: return instr.get_operand(1).get_type();
        case Opcode::TRUNCATE: return instr.get_operand(1).get_type();
        case Opcode::FPROMOTE: return instr.get_operand(1).get_type();
        case Opcode::FDEMOTE: return instr.get_operand(1).get_type();
        case Opcode::UTOF: return instr.get_operand(1).get_type();
        case Opcode::STOF: return instr.get_operand(1).get_type();
        case Opcode::FTOU: return instr.get_operand(1).get_type();
        case Opcode::FTOS: return instr.get_operand(1).get_type();
        case Opcode::MEMBERPTR: return addr_type;
        case Opcode::OFFSETPTR: return addr_type;
        case Opcode::COPY: return Primitive::VOID;
        case Opcode::SQRT: return instr.get_operand(0).get_type();
    }
}

FunctionType get_call_func_type(Instruction &call_instr) {
    FunctionType type{
        .params = std::vector<Type>{call_instr.get_operands().size() - 1},
        .return_type = call_instr.get_operand(0).get_type(),
        .calling_conv = {},
        .variadic = false,
        .first_variadic_index = 0,
    };

    for (unsigned i = 1; i < call_instr.get_operands().size(); i++) {
        type.params[i - 1] = call_instr.get_operand(i).get_type();
    }

    if (call_instr.get_attr() == Instruction::Attribute::VARIADIC) {
        type.variadic = true;
        type.first_variadic_index = call_instr.get_attr_data();
    }

    return type;
}

} // namespace banjo::ssa
