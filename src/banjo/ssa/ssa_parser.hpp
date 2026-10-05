#ifndef BANJO_SSA_PARSER_H
#define BANJO_SSA_PARSER_H

#include "banjo/ssa/function.hpp"
#include "banjo/ssa/function_type.hpp"
#include "banjo/ssa/module.hpp"
#include "banjo/ssa/opcode.hpp"
#include "banjo/ssa/operand.hpp"
#include "banjo/ssa/type.hpp"
#include "banjo/ssa/virtual_register.hpp"
#include "banjo/utils/generic_lexer.hpp"

#include <optional>
#include <vector>

namespace banjo::ssa {

class Parser {

private:
    utils::TokenStream &tokens;
    ssa::CallingConv calling_conv;

public:
    Parser(utils::TokenStream &tokens, ssa::CallingConv calling_conv);

    Module parse();

private:
    ssa::Function *parse_func();
    std::optional<std::vector<ssa::Type>> parse_params();
    std::optional<ssa::BasicBlock> parse_block();
    std::optional<ssa::Instruction> parse_instr();
    std::optional<ssa::Opcode> parse_opcode();
    std::optional<ssa::Operand> parse_operand();
    std::optional<ssa::VirtualRegister> parse_reg();

    std::optional<ssa::Type> parse_type();
    std::optional<std::string> parse_ident();

    void report_unexpected(const std::string &expected);
    void report_unexpected();
    void report_error(const std::string &message);
};

} // namespace banjo::ssa

#endif
