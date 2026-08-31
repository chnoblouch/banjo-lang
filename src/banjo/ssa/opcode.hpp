#ifndef BANJO_SSA_OPCODE_H
#define BANJO_SSA_OPCODE_H

namespace banjo::ssa {

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
    LSHL,
    LSHR,
    ASHR,
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
    FRAME_ADDRESS,
};

} // namespace banjo::ssa

#endif
