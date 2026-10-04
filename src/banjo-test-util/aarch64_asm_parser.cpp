#include "aarch64_asm_parser.hpp"

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

namespace banjo::test::assembler {

// clang-format off
static const HashMap<std::string_view, mcode::Opcode> OPCODES{
    {"mov", target::AArch64Opcode::MOV},
    {"movz", target::AArch64Opcode::MOVZ},
    {"movk", target::AArch64Opcode::MOVK},
    {"ldr", target::AArch64Opcode::LDR},
    {"ldrb", target::AArch64Opcode::LDRB},
    {"ldrh", target::AArch64Opcode::LDRH},
    {"str", target::AArch64Opcode::STR},
    {"strb", target::AArch64Opcode::STRB},
    {"strh", target::AArch64Opcode::STRH},
    {"ldp", target::AArch64Opcode::LDP},
    {"stp", target::AArch64Opcode::STP},
    {"ldar", target::AArch64Opcode::LDAR},
    {"ldarb", target::AArch64Opcode::LDARB},
    {"ldarh", target::AArch64Opcode::LDARH},
    {"ldaxr", target::AArch64Opcode::LDAXR},
    {"ldaxrb", target::AArch64Opcode::LDAXRB},
    {"ldaxrh", target::AArch64Opcode::LDAXRH},
    {"stlr", target::AArch64Opcode::STLR},
    {"stlrb", target::AArch64Opcode::STLRB},
    {"stlrh", target::AArch64Opcode::STLRH},
    {"stlxr", target::AArch64Opcode::STLXR},
    {"stlxrb", target::AArch64Opcode::STLXRB},
    {"stlxrh", target::AArch64Opcode::STLXRH},
    {"add", target::AArch64Opcode::ADD},
    {"sub", target::AArch64Opcode::SUB},
    {"mul", target::AArch64Opcode::MUL},
    {"madd", target::AArch64Opcode::MADD},
    {"msub", target::AArch64Opcode::MSUB},
    {"sdiv", target::AArch64Opcode::SDIV},
    {"udiv", target::AArch64Opcode::UDIV},
    {"and", target::AArch64Opcode::AND},
    {"orr", target::AArch64Opcode::ORR},
    {"eor", target::AArch64Opcode::EOR},
    {"lsl", target::AArch64Opcode::LSL},
    {"lsr", target::AArch64Opcode::LSR},
    {"asr", target::AArch64Opcode::ASR},
    {"csel", target::AArch64Opcode::CSEL},
    {"fmov", target::AArch64Opcode::FMOV},
    {"fadd", target::AArch64Opcode::FADD},
    {"fsub", target::AArch64Opcode::FSUB},
    {"fmul", target::AArch64Opcode::FMUL},
    {"fdiv", target::AArch64Opcode::FDIV},
    {"fcvt", target::AArch64Opcode::FCVT},
    {"scvtf", target::AArch64Opcode::SCVTF},
    {"ucvtf", target::AArch64Opcode::UCVTF},
    {"fcvtzs", target::AArch64Opcode::FCVTZS},
    {"fcvtzu", target::AArch64Opcode::FCVTZU},
    {"fcsel", target::AArch64Opcode::FCSEL},
    {"cmp", target::AArch64Opcode::CMP},
    {"fcmp", target::AArch64Opcode::FCMP},
    {"b", target::AArch64Opcode::B},
    {"br", target::AArch64Opcode::BR},
    {"b.eq", target::AArch64Opcode::B_EQ},
    {"b.ne", target::AArch64Opcode::B_NE},
    {"b.hs", target::AArch64Opcode::B_HS},
    {"b.lo", target::AArch64Opcode::B_LO},
    {"b.hi", target::AArch64Opcode::B_HI},
    {"b.ls", target::AArch64Opcode::B_LS},
    {"b.ge", target::AArch64Opcode::B_GE},
    {"b.lt", target::AArch64Opcode::B_LT},
    {"b.gt", target::AArch64Opcode::B_GT},
    {"b.le", target::AArch64Opcode::B_LE},
    {"bl", target::AArch64Opcode::BL},
    {"blr", target::AArch64Opcode::BLR},
    {"ret", target::AArch64Opcode::RET},
    {"adrp", target::AArch64Opcode::ADRP},
    {"uxtb", target::AArch64Opcode::UXTB},
    {"uxth", target::AArch64Opcode::UXTH},
    {"sxtb", target::AArch64Opcode::SXTB},
    {"sxth", target::AArch64Opcode::SXTH},
    {"sxtw", target::AArch64Opcode::SXTW},
};
// clang-format on

// clang-format off
static const HashMap<std::string_view, std::pair<mcode::PhysicalReg, unsigned>> REGISTERS{
    {"w0", {target::AArch64Register::R0, 4}},
    {"w1", {target::AArch64Register::R1, 4}},
    {"w2", {target::AArch64Register::R2, 4}},
    {"w3", {target::AArch64Register::R3, 4}},
    {"w4", {target::AArch64Register::R4, 4}},
    {"w5", {target::AArch64Register::R5, 4}},
    {"w6", {target::AArch64Register::R6, 4}},
    {"w7", {target::AArch64Register::R7, 4}},
    {"w8", {target::AArch64Register::R8, 4}},
    {"w9", {target::AArch64Register::R9, 4}},
    {"w10", {target::AArch64Register::R10, 4}},
    {"w11", {target::AArch64Register::R11, 4}},
    {"w12", {target::AArch64Register::R12, 4}},
    {"w13", {target::AArch64Register::R13, 4}},
    {"w14", {target::AArch64Register::R14, 4}},
    {"w15", {target::AArch64Register::R15, 4}},
    {"w16", {target::AArch64Register::R16, 4}},
    {"w17", {target::AArch64Register::R17, 4}},
    {"w18", {target::AArch64Register::R18, 4}},
    {"w19", {target::AArch64Register::R19, 4}},
    {"w20", {target::AArch64Register::R20, 4}},
    {"w21", {target::AArch64Register::R21, 4}},
    {"w22", {target::AArch64Register::R22, 4}},
    {"w23", {target::AArch64Register::R23, 4}},
    {"w24", {target::AArch64Register::R24, 4}},
    {"w25", {target::AArch64Register::R25, 4}},
    {"w26", {target::AArch64Register::R26, 4}},
    {"w27", {target::AArch64Register::R27, 4}},
    {"w28", {target::AArch64Register::R28, 4}},
    {"w29", {target::AArch64Register::R29, 4}},
    {"w30", {target::AArch64Register::R30, 4}},
    {"x0", {target::AArch64Register::R0, 8}},
    {"x1", {target::AArch64Register::R1, 8}},
    {"x2", {target::AArch64Register::R2, 8}},
    {"x3", {target::AArch64Register::R3, 8}},
    {"x4", {target::AArch64Register::R4, 8}},
    {"x5", {target::AArch64Register::R5, 8}},
    {"x6", {target::AArch64Register::R6, 8}},
    {"x7", {target::AArch64Register::R7, 8}},
    {"x8", {target::AArch64Register::R8, 8}},
    {"x9", {target::AArch64Register::R9, 8}},
    {"x10", {target::AArch64Register::R10, 8}},
    {"x11", {target::AArch64Register::R11, 8}},
    {"x12", {target::AArch64Register::R12, 8}},
    {"x13", {target::AArch64Register::R13, 8}},
    {"x14", {target::AArch64Register::R14, 8}},
    {"x15", {target::AArch64Register::R15, 8}},
    {"x16", {target::AArch64Register::R16, 8}},
    {"x17", {target::AArch64Register::R17, 8}},
    {"x18", {target::AArch64Register::R18, 8}},
    {"x19", {target::AArch64Register::R19, 8}},
    {"x20", {target::AArch64Register::R20, 8}},
    {"x21", {target::AArch64Register::R21, 8}},
    {"x22", {target::AArch64Register::R22, 8}},
    {"x23", {target::AArch64Register::R23, 8}},
    {"x24", {target::AArch64Register::R24, 8}},
    {"x25", {target::AArch64Register::R25, 8}},
    {"x26", {target::AArch64Register::R26, 8}},
    {"x27", {target::AArch64Register::R27, 8}},
    {"x28", {target::AArch64Register::R28, 8}},
    {"x29", {target::AArch64Register::R29, 8}},
    {"x30", {target::AArch64Register::R30, 8}},
    {"s0", {target::AArch64Register::V0, 4}},
    {"s1", {target::AArch64Register::V1, 4}},
    {"s2", {target::AArch64Register::V2, 4}},
    {"s3", {target::AArch64Register::V3, 4}},
    {"s4", {target::AArch64Register::V4, 4}},
    {"s5", {target::AArch64Register::V5, 4}},
    {"s6", {target::AArch64Register::V6, 4}},
    {"s7", {target::AArch64Register::V7, 4}},
    {"s8", {target::AArch64Register::V8, 4}},
    {"s9", {target::AArch64Register::V9, 4}},
    {"s10", {target::AArch64Register::V10, 4}},
    {"s11", {target::AArch64Register::V11, 4}},
    {"s12", {target::AArch64Register::V12, 4}},
    {"s13", {target::AArch64Register::V13, 4}},
    {"s14", {target::AArch64Register::V14, 4}},
    {"s15", {target::AArch64Register::V15, 4}},
    {"s16", {target::AArch64Register::V16, 4}},
    {"s17", {target::AArch64Register::V17, 4}},
    {"s18", {target::AArch64Register::V18, 4}},
    {"s19", {target::AArch64Register::V19, 4}},
    {"s20", {target::AArch64Register::V20, 4}},
    {"s21", {target::AArch64Register::V21, 4}},
    {"s22", {target::AArch64Register::V22, 4}},
    {"s23", {target::AArch64Register::V23, 4}},
    {"s24", {target::AArch64Register::V24, 4}},
    {"s25", {target::AArch64Register::V25, 4}},
    {"s26", {target::AArch64Register::V26, 4}},
    {"s27", {target::AArch64Register::V27, 4}},
    {"s28", {target::AArch64Register::V28, 4}},
    {"s29", {target::AArch64Register::V29, 4}},
    {"s30", {target::AArch64Register::V30, 4}},
    {"d0", {target::AArch64Register::V0, 8}},
    {"d1", {target::AArch64Register::V1, 8}},
    {"d2", {target::AArch64Register::V2, 8}},
    {"d3", {target::AArch64Register::V3, 8}},
    {"d4", {target::AArch64Register::V4, 8}},
    {"d5", {target::AArch64Register::V5, 8}},
    {"d6", {target::AArch64Register::V6, 8}},
    {"d7", {target::AArch64Register::V7, 8}},
    {"d8", {target::AArch64Register::V8, 8}},
    {"d9", {target::AArch64Register::V9, 8}},
    {"d10", {target::AArch64Register::V10, 8}},
    {"d11", {target::AArch64Register::V11, 8}},
    {"d12", {target::AArch64Register::V12, 8}},
    {"d13", {target::AArch64Register::V13, 8}},
    {"d14", {target::AArch64Register::V14, 8}},
    {"d15", {target::AArch64Register::V15, 8}},
    {"d16", {target::AArch64Register::V16, 8}},
    {"d17", {target::AArch64Register::V17, 8}},
    {"d18", {target::AArch64Register::V18, 8}},
    {"d19", {target::AArch64Register::V19, 8}},
    {"d20", {target::AArch64Register::V20, 8}},
    {"d21", {target::AArch64Register::V21, 8}},
    {"d22", {target::AArch64Register::V22, 8}},
    {"d23", {target::AArch64Register::V23, 8}},
    {"d24", {target::AArch64Register::V24, 8}},
    {"d25", {target::AArch64Register::V25, 8}},
    {"d26", {target::AArch64Register::V26, 8}},
    {"d27", {target::AArch64Register::V27, 8}},
    {"d28", {target::AArch64Register::V28, 8}},
    {"d29", {target::AArch64Register::V29, 8}},
    {"d30", {target::AArch64Register::V30, 8}},
    {"sp", {target::AArch64Register::SP, 8}},
};

// clang-format off
static const HashMap<std::string_view, target::AArch64Condition> CONDITIONS{
    {"eq", target::AArch64Condition::EQ},
    {"ne", target::AArch64Condition::NE},
    {"hs", target::AArch64Condition::HS},
    {"lo", target::AArch64Condition::LO},
    {"hi", target::AArch64Condition::HI},
    {"ls", target::AArch64Condition::LS},
    {"ge", target::AArch64Condition::GE},
    {"lt", target::AArch64Condition::LT},
    {"gt", target::AArch64Condition::GT},
    {"le", target::AArch64Condition::LE},
};
// clang-format on

AArch64AsmParser::AArch64AsmParser(TokenStream &tokens) : tokens{tokens} {}

std::optional<mcode::Instruction> AArch64AsmParser::parse_instr() {
    std::optional<mcode::Opcode> opcode = parse_opcode();
    if (!opcode) {
        return {};
    }

    if (tokens.get().type == TokenType::END_OF_LINE) {
        tokens.advance();
        return mcode::Instruction{*opcode};
    } else if (tokens.get().type == TokenType::END_OF_FILE) {
        return mcode::Instruction{*opcode};
    }

    std::vector<mcode::Operand> operands;

    while (true) {
        if (std::optional<mcode::Operand> operand = parse_operand()) {
            operands.push_back(*std::move(operand));
        } else {
            return {};
        }

        Token &token = tokens.get();

        if (token.type == TokenType::COMMA) {
            tokens.advance();
        } else if (token.type == TokenType::END_OF_LINE) {
            tokens.advance();
            break;
        } else if (token.type == TokenType::END_OF_FILE) {
            break;
        } else {
            RETURN_ERROR("expected comma or end of line, got '" + std::string{token.value} + "'");
        }
    }

    return mcode::Instruction{*opcode, std::move(operands)};
}

std::optional<mcode::Opcode> AArch64AsmParser::parse_opcode() {
    Token &token = tokens.get();

    if (token.type != TokenType::IDENTIFIER) {
        RETURN_ERROR("expected opcode, got '" + std::string{token.value} + "'");
    }

    if (const mcode::Opcode *opcode = OPCODES.try_find(token.value)) {
        tokens.advance();
        return *opcode;
    } else {
        RETURN_ERROR("unknown opcode '" + std::string{token.value} + "'");
    }
}

std::optional<mcode::Operand> AArch64AsmParser::parse_operand() {
    Token &token = tokens.get();

    if (token.type == TokenType::IDENTIFIER) {
        if (const auto *pair = REGISTERS.try_find(token.value)) {
            tokens.advance();
            mcode::Register reg = mcode::Register::from_physical(pair->first);
            return mcode::Operand::from_register(reg, pair->second);
        } else if (const auto *condition = CONDITIONS.try_find(token.value)) {
            tokens.advance();
            return mcode::Operand::from_aarch64_condition(*condition);
        } else if (token.value == "lsl") {
            tokens.advance();
            Token &shift = tokens.get();

            if (shift.type != TokenType::NUMBER) {
                RETURN_ERROR("expected number, got '" + std::string{shift.value} + "'");
            }

            tokens.advance();

            if (std::optional<std::uint64_t> value = utils::parse_u64(shift.value)) {
                return mcode::Operand::from_aarch64_left_shift(*value);
            } else {
                RETURN_ERROR("invalid shift '" + std::string{shift.value} + "'");
            }
        } else {
            RETURN_ERROR("invalid register '" + std::string{token.value} + "'");
        }
    } else if (token.type == TokenType::NUMBER) {
        // TODO: Validation
        tokens.advance();

        if (token.value.find('.') == std::string::npos) {
            return mcode::Operand::from_int_immediate(LargeInt{token.value});
        } else {
            return mcode::Operand::from_fp_immediate(std::stod(std::string{token.value}));
        }
    } else if (token.type == TokenType::LBRACKET) {
        if (std::optional<target::AArch64Address> address = parse_address()) {
            return mcode::Operand::from_aarch64_addr(*address);
        } else {
            return {};
        }
    } else {
        RETURN_ERROR("expected operand, got '" + std::string{token.value} + "'");
    }
}

std::optional<target::AArch64Address> AArch64AsmParser::parse_address() {
    // TODO: Check register sizes

    tokens.advance();

    std::optional<mcode::Register> base = parse_register();
    if (!base) {
        return {};
    }

    Token &next = tokens.get();

    if (next.type == TokenType::RBRACKET) {
        tokens.advance();
        return target::AArch64Address::new_base(*base);
    } else if (next.type == TokenType::COMMA) {
        tokens.advance();
        Token &next = tokens.get();

        if (next.type == TokenType::IDENTIFIER) {
            std::optional<mcode::Register> offset = parse_register();
            if (!offset) {
                return {};
            }

            Token &next = tokens.get();
            if (next.type != TokenType::RBRACKET) {
                RETURN_ERROR("expected ']', got '" + std::string{next.value} + "'");
            }

            tokens.advance();
            return target::AArch64Address::new_base_offset(*base, *offset);
        } else if (next.type == TokenType::NUMBER) {
            // TODO: Validate offset

            int offset = std::stol(std::string{next.value});
            tokens.advance();

            Token &next = tokens.get();
            if (next.type != TokenType::RBRACKET) {
                RETURN_ERROR("expected ']', got '" + std::string{next.value} + "'");
            }

            tokens.advance();

            if (tokens.get().type == TokenType::EXCLAMATION) {
                tokens.advance();
                return target::AArch64Address::new_base_offset_write(*base, offset);
            } else {
                return target::AArch64Address::new_base_offset(*base, offset);
            }
        } else {
            RETURN_ERROR("expected register or const offset, got '" + std::string{next.value} + "'");
        }
    } else {
        RETURN_ERROR("expected ']' or comma, got '" + std::string{next.value} + "'");
    }
}

std::optional<mcode::Register> AArch64AsmParser::parse_register() {
    Token &token = tokens.get();

    if (token.type != TokenType::IDENTIFIER) {
        RETURN_ERROR("expected register, got '" + std::string{token.value} + "'");
    }

    if (const auto *pair = REGISTERS.try_find(token.value)) {
        tokens.advance();
        return mcode::Register::from_physical(pair->first);
    } else {
        RETURN_ERROR("invalid register '" + std::string{token.value} + "'");
    }
}

} // namespace banjo::test::assembler
