#ifndef BANJO_SSA_PARSER_H
#define BANJO_SSA_PARSER_H

#include "banjo/ssa/basic_block.hpp"
#include "banjo/ssa/function.hpp"
#include "banjo/ssa/function_type.hpp"
#include "banjo/ssa/module.hpp"
#include "banjo/ssa/opcode.hpp"
#include "banjo/ssa/operand.hpp"
#include "banjo/ssa/type.hpp"
#include "banjo/ssa/virtual_register.hpp"
#include "banjo/utils/generic_lexer.hpp"
#include "banjo/utils/hash_map.hpp"

#include <optional>
#include <utility>
#include <vector>

namespace banjo::ssa {

class Parser {

private:
    struct Param {
        Type type;
        VirtualRegister reg;
    };

    struct OperandContext {
        unsigned func_index;
        unsigned block_index;
        unsigned instr_index;
        unsigned operand_index;

        bool operator==(const OperandContext &other) const = default;
        bool operator!=(const OperandContext &other) const = default;
    };

    utils::TokenStream &tokens;
    CallingConv calling_conv;

    OperandContext operand_context;
    std::vector<std::pair<OperandContext, std::string>> unresolved_blocks;
    HashMap<std::string, BasicBlockIter> blocks_by_name;

public:
    Parser(utils::TokenStream &tokens, ssa::CallingConv calling_conv);

    Module parse();

private:
    ssa::Function *parse_func();
    std::optional<std::vector<Param>> parse_params();
    std::optional<ssa::BasicBlock> parse_block(bool is_entry);
    std::optional<ssa::Instruction> parse_instr();
    std::optional<ssa::Opcode> parse_opcode();
    std::optional<ssa::Operand> parse_operand();
    std::optional<ssa::VirtualRegister> parse_reg();

    std::optional<ssa::Type> parse_type();
    std::optional<std::string> parse_ident();

    bool resolve_idents(OperandContext context, Function &func);
    bool resolve_idents(OperandContext context, Instruction &instr);

    void report_unexpected(const std::string &expected);
    void report_unexpected();
    void report_error(const std::string &message);
};

} // namespace banjo::ssa

#endif
