#ifndef BANJO_TARGET_X86_64_OPCODE_H
#define BANJO_TARGET_X86_64_OPCODE_H

namespace banjo {

namespace target {

namespace X8664ConditionCode {

enum {
    E,
    NE,
    A,
    AE,
    B,
    BE,
    G,
    GE,
    L,
    LE,
};

} // namespace X8664ConditionCode

namespace X8664Opcode {

enum {
    MOV,
    PUSH,
    POP,
    ADD,
    SUB,
    IMUL,
    DIV,
    IDIV,
    AND,
    OR,
    XOR,
    SHL,
    SHR,
    CDQ,
    CQO,
    JMP,
    CMP,
    JCC,
    JE = JCC + X8664ConditionCode::E,
    JNE = JCC + X8664ConditionCode::NE,
    JA = JCC + X8664ConditionCode::A,
    JAE = JCC + X8664ConditionCode::AE,
    JB = JCC + X8664ConditionCode::B,
    JBE = JCC + X8664ConditionCode::BE,
    JG = JCC + X8664ConditionCode::G,
    JGE = JCC + X8664ConditionCode::GE,
    JL = JCC + X8664ConditionCode::L,
    JLE = JCC + X8664ConditionCode::LE,
    CMOVCC,
    CMOVE = CMOVCC + X8664ConditionCode::E,
    CMOVNE = CMOVCC + X8664ConditionCode::NE,
    CMOVA = CMOVCC + X8664ConditionCode::A,
    CMOVAE = CMOVCC + X8664ConditionCode::AE,
    CMOVB = CMOVCC + X8664ConditionCode::B,
    CMOVBE = CMOVCC + X8664ConditionCode::BE,
    CMOVG = CMOVCC + X8664ConditionCode::G,
    CMOVGE = CMOVCC + X8664ConditionCode::GE,
    CMOVL = CMOVCC + X8664ConditionCode::L,
    CMOVLE = CMOVCC + X8664ConditionCode::LE,
    CALL,
    RET,
    LEA,
    MOVSX,
    MOVZX,
    MOVSS,
    MOVSD,
    MOVAPS,
    MOVUPS,
    MOVD,
    MOVQ,
    ADDSS,
    ADDSD,
    SUBSS,
    SUBSD,
    MULSS,
    MULSD,
    DIVSS,
    DIVSD,
    XORPS,
    XORPD,
    MINSS,
    MINSD,
    MAXSS,
    MAXSD,
    SQRTSS,
    SQRTSD,
    UCOMISS,
    UCOMISD,
    CVTSS2SD,
    CVTSD2SS,
    CVTSI2SS,
    CVTSI2SD,
    CVTSS2SI,
    CVTSD2SI
};

} // namespace X8664Opcode

} // namespace target

} // namespace banjo

#endif
