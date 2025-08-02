#ifndef BANJO_SSA_OPCODE_H
#define BANJO_SSA_OPCODE_H

namespace banjo {

namespace ssa {

enum class Opcode {
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
    RET,
    UEXTEND,
    SEXTEND,
    TRUNCATE,
    FPROMOTE,
    FDEMOTE,
    UTOF,
    STOF,
    FTOU,
    FTOS,
    MEMBERPTR,
    OFFSETPTR,
    COPY,
    SQRT,
};

} // namespace ssa

} // namespace banjo

#endif
