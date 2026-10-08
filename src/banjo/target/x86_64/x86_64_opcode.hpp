#ifndef BANJO_TARGET_X86_64_OPCODE_H
#define BANJO_TARGET_X86_64_OPCODE_H

namespace banjo::target {

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
    Z,
    NZ,
    S,
    NS,
    O,
    NO,
};

} // namespace X8664ConditionCode

namespace X8664Opcode {

enum {
    MOV,  // TODO: Test
    PUSH, // TODO: Test
    POP,  // TODO: Test
    ADD,  // TODO: Test
    SUB,  // TODO: Test
    IMUL, // TODO: Test
    DIV,
    IDIV,
    AND, // TODO: Test
    OR,  // TODO: Test
    XOR, // TODO: Test
    SHL,
    SHR,
    SAR,
    CWD,
    CDQ,
    CQO,
    XCHG,         // TODO: Test
    LOCK_CMPXCHG, // TODO: Test
    JMP,
    CMP,  // TODO: Test
    TEST, // TODO: Test
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
    JS = JCC + X8664ConditionCode::S,
    JNS = JCC + X8664ConditionCode::NS,
    JO = JCC + X8664ConditionCode::O,
    JNO = JCC + X8664ConditionCode::NO,
    SETCC,
    SETE = SETCC + X8664ConditionCode::E,
    SETNE = SETCC + X8664ConditionCode::NE,
    SETA = SETCC + X8664ConditionCode::A,
    SETAE = SETCC + X8664ConditionCode::AE,
    SETB = SETCC + X8664ConditionCode::B,
    SETBE = SETCC + X8664ConditionCode::BE,
    SETG = SETCC + X8664ConditionCode::G,
    SETGE = SETCC + X8664ConditionCode::GE,
    SETL = SETCC + X8664ConditionCode::L,
    SETLE = SETCC + X8664ConditionCode::LE,
    SETS = SETCC + X8664ConditionCode::S,
    SETNS = SETCC + X8664ConditionCode::NS,
    SETO = SETCC + X8664ConditionCode::O,
    SETNO = SETCC + X8664ConditionCode::NO,
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
    CMOVS = CMOVCC + X8664ConditionCode::S,
    CMOVNS = CMOVCC + X8664ConditionCode::NS,
    CMOVO = CMOVCC + X8664ConditionCode::O,
    CMOVNO = CMOVCC + X8664ConditionCode::NO,
    CALL,
    RET,
    LEA,
    MOVZX,
    MOVSX,
    MOVSXD,
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
    CVTSD2SI,
};

} // namespace X8664Opcode

} // namespace banjo::target

#endif
