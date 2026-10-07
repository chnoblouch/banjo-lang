#include "x86_64_parser.hpp"

#include "banjo/mcode/instruction.hpp"
#include "banjo/mcode/register.hpp"
#include "banjo/target/x86_64/x86_64_address.hpp"
#include "banjo/target/x86_64/x86_64_opcode_names.hpp"
#include "banjo/target/x86_64/x86_64_register.hpp"
#include "banjo/utils/hash_map.hpp"

#include <iostream>
#include <optional>
#include <string_view>

#define RETURN_ERROR(message)                                                                                          \
    {                                                                                                                  \
        std::cerr << "error: " << message << "\n";                                                                     \
        return {};                                                                                                     \
    }

namespace banjo::target {

// clang-format off
static const HashMap<std::string_view, std::pair<mcode::PhysicalReg, unsigned>> REGISTERS{
    {"rax", {target::X8664Register::RAX, 8}},
    {"rcx", {target::X8664Register::RCX, 8}},
    {"rdx", {target::X8664Register::RDX, 8}},
    {"rbx", {target::X8664Register::RBX, 8}},
    {"rsi", {target::X8664Register::RSI, 8}},
    {"rdi", {target::X8664Register::RDI, 8}},
    {"rsp", {target::X8664Register::RSP, 8}},
    {"rbp", {target::X8664Register::RBP, 8}},
    {"r8", {target::X8664Register::R8, 8}},
    {"r9", {target::X8664Register::R9, 8}},
    {"r10", {target::X8664Register::R10, 8}},
    {"r11", {target::X8664Register::R11, 8}},
    {"r12", {target::X8664Register::R12, 8}},
    {"r13", {target::X8664Register::R13, 8}},
    {"r14", {target::X8664Register::R14, 8}},
    {"r15", {target::X8664Register::R15, 8}},
    {"eax", {target::X8664Register::RAX, 4}},
    {"ecx", {target::X8664Register::RCX, 4}},
    {"edx", {target::X8664Register::RDX, 4}},
    {"ebx", {target::X8664Register::RBX, 4}},
    {"esi", {target::X8664Register::RSI, 4}},
    {"edi", {target::X8664Register::RDI, 4}},
    {"esp", {target::X8664Register::RSP, 4}},
    {"ebp", {target::X8664Register::RBP, 4}},
    {"r8d", {target::X8664Register::R8, 4}},
    {"r9d", {target::X8664Register::R9, 4}},
    {"r10d", {target::X8664Register::R10, 4}},
    {"r11d", {target::X8664Register::R11, 4}},
    {"r12d", {target::X8664Register::R12, 4}},
    {"r13d", {target::X8664Register::R13, 4}},
    {"r14d", {target::X8664Register::R14, 4}},
    {"r15d", {target::X8664Register::R15, 4}},
    {"ax", {target::X8664Register::RAX, 2}},
    {"cx", {target::X8664Register::RCX, 2}},
    {"dx", {target::X8664Register::RDX, 2}},
    {"bx", {target::X8664Register::RBX, 2}},
    {"si", {target::X8664Register::RSI, 2}},
    {"di", {target::X8664Register::RDI, 2}},
    {"sp", {target::X8664Register::RSP, 2}},
    {"bp", {target::X8664Register::RBP, 2}},
    {"r8w", {target::X8664Register::R8, 2}},
    {"r9w", {target::X8664Register::R9, 2}},
    {"r10w", {target::X8664Register::R10, 2}},
    {"r11w", {target::X8664Register::R11, 2}},
    {"r12w", {target::X8664Register::R12, 2}},
    {"r13w", {target::X8664Register::R13, 2}},
    {"r14w", {target::X8664Register::R14, 2}},
    {"r15w", {target::X8664Register::R15, 2}},
    {"al", {target::X8664Register::RAX, 1}},
    {"cl", {target::X8664Register::RCX, 1}},
    {"dl", {target::X8664Register::RDX, 1}},
    {"bl", {target::X8664Register::RBX, 1}},
    {"sil", {target::X8664Register::RSI, 1}},
    {"dil", {target::X8664Register::RDI, 1}},
    {"spl", {target::X8664Register::RSP, 1}},
    {"bpl", {target::X8664Register::RBP, 1}},
    {"r8b", {target::X8664Register::R8, 1}},
    {"r9b", {target::X8664Register::R9, 1}},
    {"r10b", {target::X8664Register::R10, 1}},
    {"r11b", {target::X8664Register::R11, 1}},
    {"r12b", {target::X8664Register::R12, 1}},
    {"r13b", {target::X8664Register::R13, 1}},
    {"r14b", {target::X8664Register::R14, 1}},
    {"r15b", {target::X8664Register::R15, 1}},
    {"xmm0", {target::X8664Register::XMM0, 8}},
    {"xmm1", {target::X8664Register::XMM1, 8}},
    {"xmm2", {target::X8664Register::XMM2, 8}},
    {"xmm3", {target::X8664Register::XMM3, 8}},
    {"xmm4", {target::X8664Register::XMM4, 8}},
    {"xmm5", {target::X8664Register::XMM5, 8}},
    {"xmm6", {target::X8664Register::XMM6, 8}},
    {"xmm7", {target::X8664Register::XMM7, 8}},
    {"xmm8", {target::X8664Register::XMM8, 8}},
    {"xmm9", {target::X8664Register::XMM9, 8}},
    {"xmm10", {target::X8664Register::XMM10, 8}},
    {"xmm11", {target::X8664Register::XMM11, 8}},
    {"xmm12", {target::X8664Register::XMM12, 8}},
    {"xmm13", {target::X8664Register::XMM13, 8}},
    {"xmm14", {target::X8664Register::XMM14, 8}},
    {"xmm15", {target::X8664Register::XMM15, 8}},
};
// clang-format on

static const HashMap<std::string_view, unsigned> SIZE_SPECIFIERS{
    {"byte", 1},
    {"word", 2},
    {"dword", 4},
    {"qword", 8},
};

std::optional<mcode::Opcode> X8664Parser::parse_opcode() {
    utils::Token &token = tokens.get();

    if (token.type != utils::TokenType::IDENTIFIER) {
        RETURN_ERROR("expected opcode, got '" + std::string{token.value} + "'");
    }

    if (const mcode::Opcode *opcode = X86_64_OPCODE_NAMES.try_find_by_right(token.value)) {
        tokens.advance();
        return *opcode;
    } else {
        RETURN_ERROR("unknown opcode '" + std::string{token.value} + "'");
    }
}

std::optional<mcode::Operand> X8664Parser::parse_operand() {
    utils::Token &token = tokens.get();

    if (token.type == utils::TokenType::IDENTIFIER) {
        if (const auto *pair = REGISTERS.try_find(token.value)) {
            tokens.advance();

            mcode::Register reg = mcode::Register::from_physical(pair->first);
            return mcode::Operand::from_register(reg, pair->second);
        } else if (const unsigned *size = SIZE_SPECIFIERS.try_find(token.value)) {
            tokens.advance();

            if (tokens.get().type != utils::TokenType::LBRACKET) {
                RETURN_ERROR("expected '[', got" + std::string{token.value} + "'");
            }

            if (std::optional<target::X8664Address> address = parse_address()) {
                return mcode::Operand::from_x86_64_addr(*address, *size);
            } else {
                return {};
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
        if (std::optional<target::X8664Address> address = parse_address()) {
            return mcode::Operand::from_x86_64_addr(*address);
        } else {
            return {};
        }
    } else {
        RETURN_ERROR("expected operand, got '" + std::string{token.value} + "'");
    }
}

std::optional<target::X8664Address> X8664Parser::parse_address() {
    // TODO: Check register sizes

    tokens.advance();

    std::optional<mcode::Register> base = parse_register();
    if (!base) {
        return {};
    }

    utils::Token &next = tokens.get();

    if (next.type == utils::TokenType::RBRACKET) {
        tokens.advance();
        return target::X8664Address{.base = *base};
    } else if (next.type == utils::TokenType::PLUS) {
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

            return target::X8664Address{
                .base = *base,
                .offset_reg = target::X8664Address::RegOffset{*offset},
            };

        } else {
            RETURN_ERROR("expected number or register, got '" + std::string{next.value} + "'");
        }
    } else {
        RETURN_ERROR("expected ']' or '+', got '" + std::string{next.value} + "'");
    }
}

std::optional<mcode::Register> X8664Parser::parse_register() {
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
