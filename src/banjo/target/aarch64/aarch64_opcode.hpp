#ifndef AARCH64_OPCODE_H
#define AARCH64_OPCODE_H

namespace banjo {

namespace target {

namespace AArch64Opcode {

enum {
    MOV,
    MOVZ,
    MOVK,
    LDR,
    LDRB,
    LDRH,
    STR,
    STRB,
    STRH,
    LDP,
    STP,
    ADD,
    SUB,
    MUL,
    SDIV,
    UDIV,
    AND,
    ORR,
    EOR,
    LSL,
    ASR,
    CSEL,
    FMOV,
    FADD,
    FSUB,
    FMUL,
    FDIV,
    FCVT,
    SCVTF,
    UCVTF,
    FCVTZS,
    FCVTZU,
    FCSEL,
    CMP,
    FCMP,
    B,
    BR,
    B_EQ,
    B_NE,
    B_HS,
    B_LO,
    B_HI,
    B_LS,
    B_GE,
    B_LT,
    B_GT,
    B_LE,
    BL,
    BLR,
    RET,
    ADRP,
    UXTB,
    UXTH,
    SXTB,
    SXTH,
    SXTW,
};

} // namespace AArch64Opcode

} // namespace target

} // namespace banjo

#endif
