#ifndef IR_OPCODE_H
#define IR_OPCODE_H

namespace banjo {

namespace ssa {

enum class Opcode {
    INVALID,
    ALLOCA,
    LOAD,
    STORE,
    LOADARG,
    ADD,
    SUB,
    MUL,
    SDIV,
    SREM,
    UDIV,
    UREM,
    FADD,
    FSUB,
    FMUL,
    FDIV,
    AND,
    OR,
    XOR,
    SHL,
    SHR,
    JMP,
    CJMP,
    FCJMP,
    SELECT,
    CALL,
    CALLINTR,
    RET,
    UEXTEND,
    SEXTEND,
    FPROMOTE,
    TRUNCATE,
    FDEMOTE,
    UTOF,
    STOF,
    FTOU,
    FTOS,
    MEMBERPTR,
    OFFSETPTR,
    COPY,
    SQRT,
    ASM
};

} // namespace ssa

} // namespace banjo

#endif
