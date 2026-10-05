#include "ssa_parser.hpp"

#include "banjo/ssa/basic_block.hpp"
#include "banjo/ssa/function.hpp"
#include "banjo/ssa/function_type.hpp"
#include "banjo/ssa/instruction.hpp"
#include "banjo/ssa/module.hpp"
#include "banjo/ssa/opcode.hpp"
#include "banjo/ssa/primitive.hpp"
#include "banjo/ssa/type.hpp"
#include "banjo/ssa/virtual_register.hpp"
#include "banjo/utils/generic_lexer.hpp"
#include "banjo/utils/hash_map.hpp"
#include "banjo/utils/linked_list.hpp"
#include "banjo/utils/utils.hpp"

#include <cstdint>
#include <iostream>
#include <optional>
#include <string_view>
#include <vector>

namespace banjo::ssa {

static const HashMap<std::string_view, ssa::Opcode> OPCODES{
    {"alloca", Opcode::ALLOCA},
    {"load", Opcode::LOAD},
    {"store", Opcode::STORE},
    {"loadarg", Opcode::LOADARG},
    {"add", Opcode::ADD},
    {"sub", Opcode::SUB},
    {"mul", Opcode::MUL},
    {"sdiv", Opcode::SDIV},
    {"srem", Opcode::SREM},
    {"udiv", Opcode::UDIV},
    {"urem", Opcode::UREM},
    {"fadd", Opcode::FADD},
    {"fsub", Opcode::FSUB},
    {"fmul", Opcode::FMUL},
    {"fdiv", Opcode::FDIV},
    {"and", Opcode::AND},
    {"or", Opcode::OR},
    {"xor", Opcode::XOR},
    {"lshl", Opcode::LSHL},
    {"lshr", Opcode::LSHR},
    {"ashr", Opcode::ASHR},
    {"jmp", Opcode::JMP},
    {"cjmp", Opcode::CJMP},
    {"fcjmp", Opcode::FCJMP},
    {"select", Opcode::SELECT},
    {"call", Opcode::CALL},
    {"ret", Opcode::RET},
    {"uextend", Opcode::UEXTEND},
    {"sextend", Opcode::SEXTEND},
    {"truncate", Opcode::TRUNCATE},
    {"fpromote", Opcode::FPROMOTE},
    {"fdemote", Opcode::FDEMOTE},
    {"utof", Opcode::UTOF},
    {"stof", Opcode::STOF},
    {"ftou", Opcode::FTOU},
    {"ftos", Opcode::FTOS},
    {"bitcast", Opcode::BITCAST},
    {"atomic_load", Opcode::ATOMIC_LOAD},
    {"atomic_store", Opcode::ATOMIC_STORE},
    {"atomic_swap", Opcode::ATOMIC_SWAP},
    {"atomic_cmpswap", Opcode::ATOMIC_CMPSWAP},
    {"atomic_add", Opcode::ATOMIC_ADD},
    {"atomic_sub", Opcode::ATOMIC_SUB},
    {"atomic_and", Opcode::ATOMIC_AND},
    {"atomic_or", Opcode::ATOMIC_OR},
    {"atomic_xor", Opcode::ATOMIC_XOR},
    {"memberptr", Opcode::MEMBERPTR},
    {"offsetptr", Opcode::OFFSETPTR},
    {"copy", Opcode::COPY},
    {"sqrt", Opcode::SQRT},
    {"frame_address", Opcode::FRAME_ADDRESS},
};

static const HashMap<std::string_view, ssa::Primitive> PRIMITIVES{
    {"void", ssa::Primitive::VOID},
    {"u8", ssa::Primitive::U8},
    {"u16", ssa::Primitive::U16},
    {"u32", ssa::Primitive::U32},
    {"u64", ssa::Primitive::U64},
    {"i8", ssa::Primitive::I8},
    {"i16", ssa::Primitive::I16},
    {"i32", ssa::Primitive::I32},
    {"i64", ssa::Primitive::I64},
    {"f32", ssa::Primitive::F32},
    {"f64", ssa::Primitive::F64},
    {"addr", ssa::Primitive::ADDR},
};

Parser::Parser(utils::TokenStream &tokens, ssa::CallingConv calling_conv)
  : tokens{tokens},
    calling_conv{calling_conv} {}

ssa::Module Parser::parse() {
    ssa::Module mod;

    while (true) {
        utils::Token &token = tokens.get();

        if (token.type == utils::TokenType::IDENTIFIER) {
            if (token.value == "func") {
                if (ssa::Function *func = parse_func()) {
                    mod.add(func);
                    continue;
                } else {
                    break;
                }
            }
        } else if (token.type == utils::TokenType::END_OF_LINE) {
            tokens.advance();
            continue;
        } else if (token.type == utils::TokenType::END_OF_FILE) {
            break;
        }

        report_unexpected();
        break;
    }

    return mod;
}

ssa::Function *Parser::parse_func() {
    tokens.advance();

    std::optional<ssa::Type> return_type = parse_type();
    if (!return_type) {
        return nullptr;
    }

    std::optional<std::string> name = parse_ident();
    if (!name) {
        return nullptr;
    }

    std::optional<std::vector<ssa::Type>> params = parse_params();
    if (!params) {
        return nullptr;
    }

    if (tokens.get().type == utils::TokenType::COLON) {
        tokens.advance();
    } else {
        report_unexpected("':'");
        return nullptr;
    }

    if (tokens.get().type == utils::TokenType::END_OF_LINE) {
        tokens.advance();
    } else {
        report_unexpected("end of line");
        return nullptr;
    }

    ssa::Function *func = new ssa::Function{
        *name,
        FunctionType{
            .params = *params,
            .return_type = *return_type,
            .calling_conv = calling_conv,
            .variadic = false,
            .first_variadic_index = 0,
        },
    };

    if (std::optional<BasicBlock> block = parse_block()) {
        func->basic_blocks.append(*block);
    } else {
        delete func;
        return nullptr;
    }

    return func;
}

std::optional<std::vector<ssa::Type>> Parser::parse_params() {
    if (tokens.get().type != utils::TokenType::LPAREN) {
        report_unexpected("'('");
        return {};
    }

    tokens.advance();
    std::vector<ssa::Type> params;

    if (tokens.get().type == utils::TokenType::RPAREN) {
        tokens.advance();
        return params;
    }

    while (true) {
        std::optional<ssa::Type> type = parse_type();
        if (!type) {
            return {};
        }

        params.push_back(*type);
        utils::Token &token = tokens.get();

        if (token.type == utils::TokenType::COMMA) {
            tokens.advance();
        } else if (token.type == utils::TokenType::RPAREN) {
            tokens.advance();
            break;
        } else {
            report_unexpected("',' or ')'");
            return {};
        }
    }

    return params;
}

std::optional<ssa::BasicBlock> Parser::parse_block() {
    ssa::BasicBlock block;

    while (true) {
        if (tokens.get().type == utils::TokenType::END_OF_LINE) {
            tokens.advance();
            continue;
        } else if (tokens.get().type == utils::TokenType::END_OF_FILE) {
            break;
        }

        std::optional<ssa::Instruction> instr = parse_instr();
        if (!instr) {
            return {};
        }

        block.append(*std::move(instr));

        if (tokens.get().type == utils::TokenType::END_OF_LINE) {
            tokens.advance();
        } else if (tokens.get().type == utils::TokenType::END_OF_FILE) {
            break;
        } else {
            report_unexpected();
            return {};
        }
    }

    return block;
}

std::optional<ssa::Instruction> Parser::parse_instr() {
    std::optional<ssa::VirtualRegister> dst;

    if (tokens.get().type == utils::TokenType::PERCENT) {
        if (std::optional<ssa::VirtualRegister> reg = parse_reg()) {
            dst = reg;
        } else {
            return {};
        }

        if (tokens.get().type == utils::TokenType::EQUALS) {
            tokens.advance();
        } else {
            report_unexpected("'='");
            return {};
        }
    }

    std::optional<ssa::Opcode> opcode = parse_opcode();
    if (!opcode) {
        return {};
    }

    std::vector<ssa::Operand> operands;
    utils::Token &token = tokens.get();

    if (token.type != utils::TokenType::END_OF_LINE && token.type != utils::TokenType::END_OF_FILE) {
        while (true) {
            if (std::optional<Operand> operand = parse_operand()) {
                operands.push_back(*operand);
            } else {
                return {};
            }

            if (tokens.get().type == utils::TokenType::COMMA) {
                tokens.advance();
            } else {
                break;
            }
        }
    }

    return ssa::Instruction{*opcode, dst, operands};
}

std::optional<ssa::Opcode> Parser::parse_opcode() {
    utils::Token &token = tokens.get();

    if (token.type == utils::TokenType::IDENTIFIER) {
        if (const Opcode *opcode = OPCODES.try_find(token.value)) {
            tokens.advance();
            return *opcode;
        }
    }

    report_unexpected("opcode");
    return {};
}

std::optional<ssa::Operand> Parser::parse_operand() {
    std::optional<ssa::Type> type = parse_type();
    if (!type) {
        return {};
    }

    utils::Token &token = tokens.get();

    switch (token.type) {
        case utils::TokenType::NUMBER: {
            tokens.advance();
            LargeInt value{token.value}; // TODO: Validation
            return ssa::Operand::from_int_immediate(value, *type);
        }

        case utils::TokenType::PERCENT: {
            if (std::optional<ssa::VirtualRegister> reg = parse_reg()) {
                return ssa::Operand::from_register(*reg, *type);
            } else {
                return {};
            }
        }

        case utils::TokenType::COMMA:
        case utils::TokenType::END_OF_LINE:
        case utils::TokenType::END_OF_FILE: {
            return ssa::Operand::from_type(*type);
        }

        default: break;
    }

    report_unexpected("operand");
    return {};
}

std::optional<ssa::VirtualRegister> Parser::parse_reg() {
    if (tokens.get().type == utils::TokenType::PERCENT) {
        tokens.advance();
        utils::Token &token = tokens.get();

        if (token.type == utils::TokenType::NUMBER) {
            if (std::optional<std::uint64_t> value = utils::parse_u64(token.value)) {
                tokens.advance();
                return static_cast<ssa::VirtualRegister>(*value);
            }
        }
    }

    report_unexpected("virtual register");
    return {};
}

std::optional<ssa::Type> Parser::parse_type() {
    utils::Token &token = tokens.get();

    if (token.type == utils::TokenType::IDENTIFIER) {
        if (const ssa::Primitive *primitive = PRIMITIVES.try_find(token.value)) {
            tokens.advance();
            return *primitive;
        }
    }

    report_unexpected("type");
    return {};
}

std::optional<std::string> Parser::parse_ident() {
    if (tokens.get().type == utils::TokenType::AT) {
        tokens.advance();
        utils::Token &token = tokens.get();

        if (token.type == utils::TokenType::IDENTIFIER) {
            tokens.advance();
            return std::string{token.value};
        }
    }

    report_unexpected("identifier");
    return {};
}

void Parser::report_unexpected(const std::string &expected) {
    report_error("expected " + expected + ", got '" + std::string{tokens.get().value} + "'");
}

void Parser::report_unexpected() {
    report_error("unexpected token '" + std::string{tokens.get().value} + "'");
}

void Parser::report_error(const std::string &message) {
    std::cerr << "error: " << message << '\n';
}

} // namespace banjo::ssa
