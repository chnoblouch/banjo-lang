#include "ssa_parser.hpp"

#include "banjo/ssa/basic_block.hpp"
#include "banjo/ssa/function.hpp"
#include "banjo/ssa/function_type.hpp"
#include "banjo/ssa/instruction.hpp"
#include "banjo/ssa/module.hpp"
#include "banjo/ssa/opcode.hpp"
#include "banjo/ssa/operand.hpp"
#include "banjo/ssa/primitive.hpp"
#include "banjo/ssa/structure.hpp"
#include "banjo/ssa/type.hpp"
#include "banjo/ssa/virtual_register.hpp"
#include "banjo/utils/generic_lexer.hpp"
#include "banjo/utils/hash_map.hpp"
#include "banjo/utils/linked_list.hpp"
#include "banjo/utils/utils.hpp"

#include <cstdint>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace banjo::ssa {

static const HashMap<std::string_view, Opcode> OPCODES{
    {"alloca", Opcode::ALLOCA},
    {"load", Opcode::LOAD},
    {"store", Opcode::STORE},
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

static const HashMap<std::string_view, Primitive> PRIMITIVES{
    {"void", Primitive::VOID},
    {"u8", Primitive::U8},
    {"u16", Primitive::U16},
    {"u32", Primitive::U32},
    {"u64", Primitive::U64},
    {"i8", Primitive::I8},
    {"i16", Primitive::I16},
    {"i32", Primitive::I32},
    {"i64", Primitive::I64},
    {"f32", Primitive::F32},
    {"f64", Primitive::F64},
    {"addr", Primitive::ADDR},
};

static std::string token_to_string(utils::Token &token) {
    if (token.type == utils::TokenType::END_OF_LINE) {
        return "end of line";
    } else if (token.type == utils::TokenType::END_OF_FILE) {
        return "end of file";
    } else {
        return '\'' + std::string{token.value} + '\'';
    }
}

Parser::Parser(utils::TokenStream &tokens, CallingConv calling_conv) : tokens{tokens}, calling_conv{calling_conv} {}

Module Parser::parse() {
    Module mod;
    mod.block_id = 1000; // FIXME

    while (true) {
        utils::Token &token = tokens.get();

        if (token.type == utils::TokenType::IDENTIFIER) {
            if (token.value == "func") {
                operand_context.func_index = mod.get_functions().size();

                if (!parse_func(mod)) {
                    break;
                }
            } else if (token.value == "struct") {
                if (!parse_struct(mod)) {
                    break;
                }
            } else {
                report_unexpected();
                break;
            }
        } else if (token.type == utils::TokenType::END_OF_LINE) {
            tokens.advance();
        } else if (token.type == utils::TokenType::END_OF_FILE) {
            break;
        } else {
            report_unexpected();
            break;
        }
    }

    for (unsigned i = 0; i < mod.get_functions().size(); i++) {
        Function &func = *mod.get_functions()[i];

        if (!resolve_idents({.func_index = i}, func)) {
            return {};
        }
    }

    return mod;
}

bool Parser::parse_func(Module &mod) {
    tokens.advance();

    std::optional<Type> return_type = parse_type();
    if (!return_type) {
        return false;
    }

    std::optional<std::string> name = parse_ident();
    if (!name) {
        return false;
    }

    std::optional<std::vector<Param>> params = parse_params();
    if (!params) {
        return false;
    }

    std::vector<Type> param_types;
    std::vector<VirtualRegister> param_regs;

    param_types.reserve(params->size());
    param_regs.reserve(params->size());

    for (Param &param : *params) {
        param_types.push_back(param.type);
        param_regs.push_back(param.reg);
    }

    FunctionType func_type{
        .params = param_types,
        .return_type = *return_type,
        .calling_conv = calling_conv,
        .variadic = false,
        .first_variadic_index = 0,
    };

    bool global = false;

    if (tokens.get().type == utils::TokenType::IDENTIFIER) {
        if (tokens.get().value == "global") {
            tokens.advance();
            global = true;
        } else {
            report_unexpected();
            return false;
        }
    }

    if (tokens.get().type == utils::TokenType::LBRACE) {
        tokens.advance();
    } else if (try_end_line()) {
        FunctionDecl *extern_func = new FunctionDecl{.name = *name, .type = func_type, .global = global};
        mod.add(extern_func);
        extern_funcs_by_name.insert(*name, extern_func);
        return true;
    } else {
        report_unexpected();
        return false;
    }

    if (tokens.get().type == utils::TokenType::END_OF_LINE) {
        tokens.advance();
    } else {
        report_unexpected("end of line");
        return false;
    }

    Function *func = new Function{*name, func_type};
    func->last_virtual_reg = 1000; // FIXME
    func->global = global;

    while (true) {
        operand_context.block_index = func->basic_blocks.get_size();
        bool is_entry = func->basic_blocks.get_size() == 0;

        if (std::optional<BasicBlock> block = parse_block(is_entry)) {
            BasicBlockIter iter = func->basic_blocks.append(*std::move(block));
            blocks_by_name.insert(iter->get_label(), iter);
        } else {
            delete func;
            return false;
        }

        utils::Token &token = tokens.get();

        if (token.type == utils::TokenType::RBRACE) {
            tokens.advance();
            break;
        }
    }

    BasicBlock &entry_block = func->get_entry_block();
    entry_block.param_regs = param_regs;
    entry_block.param_types = param_types;

    mod.add(func);
    funcs_by_name.insert(func->name, func);

    return true;
}

std::optional<std::vector<Parser::Param>> Parser::parse_params() {
    if (tokens.get().type != utils::TokenType::LPAREN) {
        report_unexpected("'('");
        return {};
    }

    tokens.advance();
    std::vector<Param> params;

    if (tokens.get().type == utils::TokenType::RPAREN) {
        tokens.advance();
        return params;
    }

    while (true) {
        std::optional<Type> type = parse_type();
        if (!type) {
            return {};
        }

        if (tokens.get().type == utils::TokenType::COMMA || tokens.get().type == utils::TokenType::RPAREN) {
            // TODO: Error handling for non-external functions
            params.push_back({*type, -1});
        } else {
            std::optional<VirtualRegister> reg = parse_reg();
            if (!reg) {
                return {};
            }

            params.push_back({*type, *reg});
        }

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

std::optional<BasicBlock> Parser::parse_block(bool is_entry) {
    BasicBlock block;
    utils::Token &token = tokens.get();

    if (!is_entry) {
        std::string_view label = tokens.get().value;

        if (tokens.get().value[0] == 'b') {
            tokens.advance();
        } else {
            report_error("invalid block label '" + std::string{token.value} + "'");
            return {};
        }

        std::vector<VirtualRegister> param_regs;
        std::vector<Type> param_types;

        if (tokens.is_current(utils::TokenType::LPAREN)) {
            tokens.advance();

            while (true) {
                if (std::optional<Type> type = parse_type()) {
                    param_types.push_back(*type);
                } else {
                    return {};
                }

                if (std::optional<VirtualRegister> reg = parse_reg()) {
                    param_regs.push_back(*reg);
                } else {
                    return {};
                }

                if (tokens.is_current(utils::TokenType::COMMA)) {
                    tokens.advance();
                } else if (tokens.is_current(utils::TokenType::RPAREN)) {
                    tokens.advance();
                    break;
                } else {
                    report_unexpected("',' or ')'");
                    return {};
                }
            }
        }

        block = BasicBlock{std::string{label}};
        block.param_regs = param_regs;
        block.param_types = param_types;

        if (tokens.is_current(utils::TokenType::COLON)) {
            tokens.advance();
        } else {
            report_unexpected("':'");
            return {};
        }
    }

    while (true) {
        if (tokens.get().type == utils::TokenType::END_OF_LINE) {
            tokens.advance();
            continue;
        } else if (tokens.get().type == utils::TokenType::RBRACE) {
            break;
        }

        if (tokens.is_current(utils::TokenType::IDENTIFIER)) {
            if (tokens.is_next(utils::TokenType::COLON) || tokens.is_next(utils::TokenType::LPAREN)) {
                break;
            }
        }

        operand_context.instr_index = block.get_instrs().get_size();

        std::optional<Instruction> instr = parse_instr();
        if (!instr) {
            return {};
        }

        block.append(*std::move(instr));

        if (tokens.get().type == utils::TokenType::END_OF_LINE) {
            tokens.advance();
        } else {
            report_unexpected();
            return {};
        }
    }

    return block;
}

std::optional<Instruction> Parser::parse_instr() {
    std::optional<VirtualRegister> dst;

    if (tokens.get().type == utils::TokenType::PERCENT) {
        if (std::optional<VirtualRegister> reg = parse_reg()) {
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

    std::optional<Opcode> opcode = parse_opcode();
    if (!opcode) {
        return {};
    }

    std::vector<Operand> operands;
    utils::Token &token = tokens.get();

    if (token.type != utils::TokenType::END_OF_LINE && token.type != utils::TokenType::END_OF_FILE) {
        while (true) {
            operand_context.operand_index = operands.size();

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

    return Instruction{*opcode, dst, operands};
}

std::optional<Opcode> Parser::parse_opcode() {
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

std::optional<Operand> Parser::parse_operand() {
    std::optional<Type> type = parse_type();
    if (!type) {
        return {};
    }

    utils::Token &token = tokens.get();

    switch (token.type) {
        case utils::TokenType::NUMBER: {
            tokens.advance();

            if (token.value.find('.') == std::string::npos) {
                LargeInt value{token.value}; // TODO: Validation
                return Operand::from_int_immediate(value, *type);
            } else {
                double value = std::stod(std::string{token.value});
                return Operand::from_fp_immediate(value, *type);
            }
        }

        case utils::TokenType::PERCENT: {
            if (std::optional<VirtualRegister> reg = parse_reg()) {
                return Operand::from_register(*reg, *type);
            } else {
                return {};
            }
        }

        case utils::TokenType::IDENTIFIER: {
            if (std::optional<BranchTarget> branch_target = parse_branch_target()) {
                return Operand::from_branch_target(*branch_target, *type);
            } else {
                return {};
            }
        }

        case utils::TokenType::AT: {
            std::optional<std::string> ident = parse_ident();
            if (!ident) {
                return {};
            }

            if (Function **func = funcs_by_name.try_find(*ident)) {
                return Operand::from_func(*func, *type);
            }

            if (FunctionDecl **extern_func = extern_funcs_by_name.try_find(*ident)) {
                return Operand::from_extern_func(*extern_func, *type);
            }

            report_error("cannot find symbol '" + *ident + "'");
            return {};
        }

        case utils::TokenType::COMMA:
        case utils::TokenType::END_OF_LINE:
        case utils::TokenType::END_OF_FILE: {
            return Operand::from_type(*type);
        }

        default: break;
    }

    report_unexpected("operand");
    return {};
}

std::optional<BranchTarget> Parser::parse_branch_target() {
    if (!tokens.is_current(utils::TokenType::IDENTIFIER)) {
        report_unexpected("block label");
        return {};
    }

    std::string_view label = tokens.get().value;

    if (label[0] == 'b') {
        tokens.advance();
    } else {
        report_error("invalid block label '" + std::string{label} + "'");
        return {};
    }

    std::vector<Value> args;

    if (tokens.is_current(utils::TokenType::LPAREN)) {
        tokens.advance();

        while (true) {
            if (std::optional<Value> arg = parse_operand()) {
                args.push_back(*arg);
            } else {
                return {};
            }

            if (tokens.is_current(utils::TokenType::COMMA)) {
                tokens.advance();
            } else if (tokens.is_current(utils::TokenType::RPAREN)) {
                tokens.advance();
                break;
            } else {
                report_unexpected("',' or ')'");
                return {};
            }
        }
    }

    unresolved_blocks.push_back({operand_context, std::string{label}});
    return BranchTarget{.block = nullptr, .args = args};
}

std::optional<VirtualRegister> Parser::parse_reg() {
    if (tokens.get().type == utils::TokenType::PERCENT) {
        tokens.advance();
        utils::Token &token = tokens.get();

        if (token.type == utils::TokenType::NUMBER) {
            if (std::optional<std::uint64_t> value = utils::parse_u64(token.value)) {
                tokens.advance();
                return static_cast<VirtualRegister>(*value);
            }
        }
    }

    report_unexpected("virtual register");
    return {};
}

bool Parser::parse_struct(Module &mod) {
    tokens.advance();

    std::optional<std::string> name = parse_ident();
    if (!name) {
        return false;
    }

    if (tokens.get().type == utils::TokenType::LBRACE) {
        tokens.advance();
    } else {
        report_unexpected("'{'");
        return false;
    }

    if (tokens.get().type == utils::TokenType::END_OF_LINE) {
        tokens.advance();
    } else {
        report_unexpected("end of line");
        return false;
    }

    std::vector<StructureMember> members;

    while (true) {
        if (tokens.get().type == utils::TokenType::END_OF_LINE) {
            continue;
        } else if (tokens.get().type == utils::TokenType::RBRACE) {
            tokens.advance();
            break;
        }

        std::optional<Type> type = parse_type();
        if (!type) {
            return false;
        }

        std::optional<std::string> name = parse_ident();
        if (!name) {
            return false;
        }

        if (tokens.get().type == utils::TokenType::END_OF_LINE) {
            tokens.advance();
        } else {
            report_unexpected("end of line");
            return false;
        }

        members.push_back(StructureMember{*name, *type});
    }

    Structure *struct_ = new Structure{*name};
    struct_->members = members;

    mod.add(struct_);
    structs_by_name.insert(struct_->name, struct_);

    return true;
}

std::optional<Type> Parser::parse_type() {
    utils::Token &token = tokens.get();

    if (token.type == utils::TokenType::IDENTIFIER) {
        if (const Primitive *primitive = PRIMITIVES.try_find(token.value)) {
            tokens.advance();
            return *primitive;
        }
    } else if (token.type == utils::TokenType::AT) {
        if (std::optional<std::string> ident = parse_ident()) {
            if (Structure **struct_ = structs_by_name.try_find(*ident)) {
                return *struct_;
            } else {
                report_error("cannot find struct '" + *ident + "'");
                return {};
            }
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

bool Parser::try_end_line() {
    utils::TokenType type = tokens.get().type;

    if (type == utils::TokenType::END_OF_LINE) {
        tokens.advance();
        return true;
    } else if (type == utils::TokenType::END_OF_FILE) {
        return true;
    } else {
        return false;
    }
}

bool Parser::resolve_idents(OperandContext context, Function &func) {
    context.block_index = 0;

    for (BasicBlock &block : func.basic_blocks) {
        context.instr_index = 0;

        for (Instruction &instr : block.get_instrs()) {
            if (!resolve_idents(context, instr)) {
                return false;
            }

            context.instr_index += 1;
        }

        context.block_index += 1;
    }

    return true;
}

bool Parser::resolve_idents(OperandContext context, Instruction &instr) {
    for (unsigned i = 0; i < instr.get_operands().size(); i++) {
        Operand &operand = instr.get_operand(i);
        context.operand_index = i;

        for (const auto &[placeholder_context, block_name] : unresolved_blocks) {
            if (placeholder_context != context) {
                continue;
            }

            if (BasicBlockIter *block = blocks_by_name.try_find(block_name)) {
                operand.get_branch_target().block = *block;
            } else {
                report_error("cannot find block '" + block_name + "'");
                return false;
            }
        }
    }

    return true;
}

void Parser::report_unexpected(const std::string &expected) {
    report_error("expected " + expected + ", got " + token_to_string(tokens.get()));
}

void Parser::report_unexpected() {
    report_error("unexpected token " + token_to_string(tokens.get()));
}

void Parser::report_error(const std::string &message) {
    std::cerr << "error: " << message << '\n';
}

} // namespace banjo::ssa
