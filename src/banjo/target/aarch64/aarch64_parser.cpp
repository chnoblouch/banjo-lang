#include "aarch64_parser.hpp"

#include "banjo/mcode/instruction.hpp"
#include "banjo/mcode/register.hpp"
#include "banjo/target/aarch64/aarch64_address.hpp"
#include "banjo/target/aarch64/aarch64_opcode.hpp"
#include "banjo/target/aarch64/aarch64_register.hpp"
#include "banjo/utils/hash_map.hpp"
#include "banjo/utils/utils.hpp"

#include <iostream>
#include <optional>

#define RETURN_ERROR(message)                                                                                          \
    {                                                                                                                  \
        std::cerr << "error: " << message << "\n";                                                                     \
        return {};                                                                                                     \
    }

namespace banjo::target {

// clang-format off
static const HashMap<std::string_view, mcode::Opcode> OPCODES{
    {"mov", AArch64Opcode::MOV},
    {"movz", AArch64Opcode::MOVZ},
    {"movk", AArch64Opcode::MOVK},
    {"ldr", AArch64Opcode::LDR},
    {"ldrb", AArch64Opcode::LDRB},
    {"ldrh", AArch64Opcode::LDRH},
    {"str", AArch64Opcode::STR},
    {"strb", AArch64Opcode::STRB},
    {"strh", AArch64Opcode::STRH},
    {"ldp", AArch64Opcode::LDP},
    {"stp", AArch64Opcode::STP},
    {"ldar", AArch64Opcode::LDAR},
    {"ldarb", AArch64Opcode::LDARB},
    {"ldarh", AArch64Opcode::LDARH},
    {"ldaxr", AArch64Opcode::LDAXR},
    {"ldaxrb", AArch64Opcode::LDAXRB},
    {"ldaxrh", AArch64Opcode::LDAXRH},
    {"stlr", AArch64Opcode::STLR},
    {"stlrb", AArch64Opcode::STLRB},
    {"stlrh", AArch64Opcode::STLRH},
    {"stlxr", AArch64Opcode::STLXR},
    {"stlxrb", AArch64Opcode::STLXRB},
    {"stlxrh", AArch64Opcode::STLXRH},
    {"add", AArch64Opcode::ADD},
    {"sub", AArch64Opcode::SUB},
    {"mul", AArch64Opcode::MUL},
    {"madd", AArch64Opcode::MADD},
    {"msub", AArch64Opcode::MSUB},
    {"sdiv", AArch64Opcode::SDIV},
    {"udiv", AArch64Opcode::UDIV},
    {"and", AArch64Opcode::AND},
    {"orr", AArch64Opcode::ORR},
    {"eor", AArch64Opcode::EOR},
    {"lsl", AArch64Opcode::LSL},
    {"lsr", AArch64Opcode::LSR},
    {"asr", AArch64Opcode::ASR},
    {"csel", AArch64Opcode::CSEL},
    {"fmov", AArch64Opcode::FMOV},
    {"fadd", AArch64Opcode::FADD},
    {"fsub", AArch64Opcode::FSUB},
    {"fmul", AArch64Opcode::FMUL},
    {"fdiv", AArch64Opcode::FDIV},
    {"fcvt", AArch64Opcode::FCVT},
    {"scvtf", AArch64Opcode::SCVTF},
    {"ucvtf", AArch64Opcode::UCVTF},
    {"fcvtzs", AArch64Opcode::FCVTZS},
    {"fcvtzu", AArch64Opcode::FCVTZU},
    {"fcsel", AArch64Opcode::FCSEL},
    {"cmp", AArch64Opcode::CMP},
    {"fcmp", AArch64Opcode::FCMP},
    {"b", AArch64Opcode::B},
    {"br", AArch64Opcode::BR},
    {"b.eq", AArch64Opcode::B_EQ},
    {"b.ne", AArch64Opcode::B_NE},
    {"b.hs", AArch64Opcode::B_HS},
    {"b.lo", AArch64Opcode::B_LO},
    {"b.hi", AArch64Opcode::B_HI},
    {"b.ls", AArch64Opcode::B_LS},
    {"b.ge", AArch64Opcode::B_GE},
    {"b.lt", AArch64Opcode::B_LT},
    {"b.gt", AArch64Opcode::B_GT},
    {"b.le", AArch64Opcode::B_LE},
    {"bl", AArch64Opcode::BL},
    {"blr", AArch64Opcode::BLR},
    {"ret", AArch64Opcode::RET},
    {"adrp", AArch64Opcode::ADRP},
    {"uxtb", AArch64Opcode::UXTB},
    {"uxth", AArch64Opcode::UXTH},
    {"sxtb", AArch64Opcode::SXTB},
    {"sxth", AArch64Opcode::SXTH},
    {"sxtw", AArch64Opcode::SXTW},
};
// clang-format on

// clang-format off
static const HashMap<std::string_view, std::pair<mcode::PhysicalReg, unsigned>> REGISTERS{
    {"w0", {AArch64Register::R0, 4}},
    {"w1", {AArch64Register::R1, 4}},
    {"w2", {AArch64Register::R2, 4}},
    {"w3", {AArch64Register::R3, 4}},
    {"w4", {AArch64Register::R4, 4}},
    {"w5", {AArch64Register::R5, 4}},
    {"w6", {AArch64Register::R6, 4}},
    {"w7", {AArch64Register::R7, 4}},
    {"w8", {AArch64Register::R8, 4}},
    {"w9", {AArch64Register::R9, 4}},
    {"w10", {AArch64Register::R10, 4}},
    {"w11", {AArch64Register::R11, 4}},
    {"w12", {AArch64Register::R12, 4}},
    {"w13", {AArch64Register::R13, 4}},
    {"w14", {AArch64Register::R14, 4}},
    {"w15", {AArch64Register::R15, 4}},
    {"w16", {AArch64Register::R16, 4}},
    {"w17", {AArch64Register::R17, 4}},
    {"w18", {AArch64Register::R18, 4}},
    {"w19", {AArch64Register::R19, 4}},
    {"w20", {AArch64Register::R20, 4}},
    {"w21", {AArch64Register::R21, 4}},
    {"w22", {AArch64Register::R22, 4}},
    {"w23", {AArch64Register::R23, 4}},
    {"w24", {AArch64Register::R24, 4}},
    {"w25", {AArch64Register::R25, 4}},
    {"w26", {AArch64Register::R26, 4}},
    {"w27", {AArch64Register::R27, 4}},
    {"w28", {AArch64Register::R28, 4}},
    {"w29", {AArch64Register::R29, 4}},
    {"w30", {AArch64Register::R30, 4}},
    {"x0", {AArch64Register::R0, 8}},
    {"x1", {AArch64Register::R1, 8}},
    {"x2", {AArch64Register::R2, 8}},
    {"x3", {AArch64Register::R3, 8}},
    {"x4", {AArch64Register::R4, 8}},
    {"x5", {AArch64Register::R5, 8}},
    {"x6", {AArch64Register::R6, 8}},
    {"x7", {AArch64Register::R7, 8}},
    {"x8", {AArch64Register::R8, 8}},
    {"x9", {AArch64Register::R9, 8}},
    {"x10", {AArch64Register::R10, 8}},
    {"x11", {AArch64Register::R11, 8}},
    {"x12", {AArch64Register::R12, 8}},
    {"x13", {AArch64Register::R13, 8}},
    {"x14", {AArch64Register::R14, 8}},
    {"x15", {AArch64Register::R15, 8}},
    {"x16", {AArch64Register::R16, 8}},
    {"x17", {AArch64Register::R17, 8}},
    {"x18", {AArch64Register::R18, 8}},
    {"x19", {AArch64Register::R19, 8}},
    {"x20", {AArch64Register::R20, 8}},
    {"x21", {AArch64Register::R21, 8}},
    {"x22", {AArch64Register::R22, 8}},
    {"x23", {AArch64Register::R23, 8}},
    {"x24", {AArch64Register::R24, 8}},
    {"x25", {AArch64Register::R25, 8}},
    {"x26", {AArch64Register::R26, 8}},
    {"x27", {AArch64Register::R27, 8}},
    {"x28", {AArch64Register::R28, 8}},
    {"x29", {AArch64Register::R29, 8}},
    {"x30", {AArch64Register::R30, 8}},
    {"s0", {AArch64Register::V0, 4}},
    {"s1", {AArch64Register::V1, 4}},
    {"s2", {AArch64Register::V2, 4}},
    {"s3", {AArch64Register::V3, 4}},
    {"s4", {AArch64Register::V4, 4}},
    {"s5", {AArch64Register::V5, 4}},
    {"s6", {AArch64Register::V6, 4}},
    {"s7", {AArch64Register::V7, 4}},
    {"s8", {AArch64Register::V8, 4}},
    {"s9", {AArch64Register::V9, 4}},
    {"s10", {AArch64Register::V10, 4}},
    {"s11", {AArch64Register::V11, 4}},
    {"s12", {AArch64Register::V12, 4}},
    {"s13", {AArch64Register::V13, 4}},
    {"s14", {AArch64Register::V14, 4}},
    {"s15", {AArch64Register::V15, 4}},
    {"s16", {AArch64Register::V16, 4}},
    {"s17", {AArch64Register::V17, 4}},
    {"s18", {AArch64Register::V18, 4}},
    {"s19", {AArch64Register::V19, 4}},
    {"s20", {AArch64Register::V20, 4}},
    {"s21", {AArch64Register::V21, 4}},
    {"s22", {AArch64Register::V22, 4}},
    {"s23", {AArch64Register::V23, 4}},
    {"s24", {AArch64Register::V24, 4}},
    {"s25", {AArch64Register::V25, 4}},
    {"s26", {AArch64Register::V26, 4}},
    {"s27", {AArch64Register::V27, 4}},
    {"s28", {AArch64Register::V28, 4}},
    {"s29", {AArch64Register::V29, 4}},
    {"s30", {AArch64Register::V30, 4}},
    {"d0", {AArch64Register::V0, 8}},
    {"d1", {AArch64Register::V1, 8}},
    {"d2", {AArch64Register::V2, 8}},
    {"d3", {AArch64Register::V3, 8}},
    {"d4", {AArch64Register::V4, 8}},
    {"d5", {AArch64Register::V5, 8}},
    {"d6", {AArch64Register::V6, 8}},
    {"d7", {AArch64Register::V7, 8}},
    {"d8", {AArch64Register::V8, 8}},
    {"d9", {AArch64Register::V9, 8}},
    {"d10", {AArch64Register::V10, 8}},
    {"d11", {AArch64Register::V11, 8}},
    {"d12", {AArch64Register::V12, 8}},
    {"d13", {AArch64Register::V13, 8}},
    {"d14", {AArch64Register::V14, 8}},
    {"d15", {AArch64Register::V15, 8}},
    {"d16", {AArch64Register::V16, 8}},
    {"d17", {AArch64Register::V17, 8}},
    {"d18", {AArch64Register::V18, 8}},
    {"d19", {AArch64Register::V19, 8}},
    {"d20", {AArch64Register::V20, 8}},
    {"d21", {AArch64Register::V21, 8}},
    {"d22", {AArch64Register::V22, 8}},
    {"d23", {AArch64Register::V23, 8}},
    {"d24", {AArch64Register::V24, 8}},
    {"d25", {AArch64Register::V25, 8}},
    {"d26", {AArch64Register::V26, 8}},
    {"d27", {AArch64Register::V27, 8}},
    {"d28", {AArch64Register::V28, 8}},
    {"d29", {AArch64Register::V29, 8}},
    {"d30", {AArch64Register::V30, 8}},
    {"sp", {AArch64Register::SP, 8}},
};

// clang-format off
static const HashMap<std::string_view, AArch64Condition> CONDITIONS{
    {"eq", AArch64Condition::EQ},
    {"ne", AArch64Condition::NE},
    {"hs", AArch64Condition::HS},
    {"lo", AArch64Condition::LO},
    {"hi", AArch64Condition::HI},
    {"ls", AArch64Condition::LS},
    {"ge", AArch64Condition::GE},
    {"lt", AArch64Condition::LT},
    {"gt", AArch64Condition::GT},
    {"le", AArch64Condition::LE},
};
// clang-format on

std::optional<mcode::Opcode> AArch64Parser::parse_opcode() {
    utils::Token &token = tokens.get();

    if (token.type != utils::TokenType::IDENTIFIER) {
        RETURN_ERROR("expected opcode, got '" + std::string{token.value} + "'");
    }

    if (const mcode::Opcode *opcode = OPCODES.try_find(token.value)) {
        tokens.advance();
        return *opcode;
    } else {
        RETURN_ERROR("unknown opcode '" + std::string{token.value} + "'");
    }
}

std::optional<mcode::Operand> AArch64Parser::parse_operand() {
    utils::Token &token = tokens.get();

    if (token.type == utils::TokenType::IDENTIFIER) {
        if (const auto *pair = REGISTERS.try_find(token.value)) {
            tokens.advance();
            mcode::Register reg = mcode::Register::from_physical(pair->first);
            return mcode::Operand::from_register(reg, pair->second);
        } else if (const auto *condition = CONDITIONS.try_find(token.value)) {
            tokens.advance();
            return mcode::Operand::from_aarch64_condition(*condition);
        } else if (token.value == "lsl") {
            tokens.advance();
            utils::Token &shift = tokens.get();

            if (shift.type != utils::TokenType::NUMBER) {
                RETURN_ERROR("expected number, got '" + std::string{shift.value} + "'");
            }

            tokens.advance();

            if (std::optional<std::uint64_t> value = utils::parse_u64(shift.value)) {
                return mcode::Operand::from_aarch64_left_shift(*value);
            } else {
                RETURN_ERROR("invalid shift '" + std::string{shift.value} + "'");
            }
        } else {
            return parse_ident_operand();
        }
    } else if (token.type == utils::TokenType::NUMBER) {
        // TODO: Validation
        tokens.advance();

        if (token.value.find('.') == std::string::npos) {
            return mcode::Operand::from_int_immediate(LargeInt{token.value});
        } else {
            return mcode::Operand::from_fp_immediate(std::stod(std::string{token.value}));
        }
    } else if (token.type == utils::TokenType::LBRACKET) {
        if (std::optional<AArch64Address> address = parse_address()) {
            return mcode::Operand::from_aarch64_addr(*address);
        } else {
            return {};
        }
    } else {
        RETURN_ERROR("expected operand, got '" + std::string{token.value} + "'");
    }
}

std::optional<AArch64Address> AArch64Parser::parse_address() {
    // TODO: Check register sizes

    tokens.advance();

    std::optional<mcode::Register> base = parse_register();
    if (!base) {
        return {};
    }

    utils::Token &next = tokens.get();

    if (next.type == utils::TokenType::RBRACKET) {
        tokens.advance();
        return AArch64Address::new_base(*base);
    } else if (next.type == utils::TokenType::COMMA) {
        tokens.advance();
        utils::Token &next = tokens.get();

        if (next.type == utils::TokenType::IDENTIFIER) {
            std::optional<mcode::Register> offset = parse_register();
            if (!offset) {
                return {};
            }

            utils::Token &next = tokens.get();
            if (next.type != utils::TokenType::RBRACKET) {
                RETURN_ERROR("expected ']', got '" + std::string{next.value} + "'");
            }

            tokens.advance();
            return AArch64Address::new_base_offset(*base, *offset);
        } else if (next.type == utils::TokenType::NUMBER) {
            // TODO: Validate offset

            int offset = std::stol(std::string{next.value});
            tokens.advance();

            utils::Token &next = tokens.get();
            if (next.type != utils::TokenType::RBRACKET) {
                RETURN_ERROR("expected ']', got '" + std::string{next.value} + "'");
            }

            tokens.advance();

            if (tokens.get().type == utils::TokenType::EXCLAMATION) {
                tokens.advance();
                return AArch64Address::new_base_offset_write(*base, offset);
            } else {
                return AArch64Address::new_base_offset(*base, offset);
            }
        } else {
            RETURN_ERROR("expected register or const offset, got '" + std::string{next.value} + "'");
        }
    } else {
        RETURN_ERROR("expected ']' or comma, got '" + std::string{next.value} + "'");
    }
}

std::optional<mcode::Register> AArch64Parser::parse_register() {
    utils::Token &token = tokens.get();

    if (token.type != utils::TokenType::IDENTIFIER) {
        RETURN_ERROR("expected register, got '" + std::string{token.value} + "'");
    }

    if (const auto *pair = REGISTERS.try_find(token.value)) {
        tokens.advance();
        return mcode::Register::from_physical(pair->first);
    } else {
        RETURN_ERROR("invalid register '" + std::string{token.value} + "'");
    }
}

} // namespace banjo::target
