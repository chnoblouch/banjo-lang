#ifndef BANJO_MCODE_PARSER_H
#define BANJO_MCODE_PARSER_H

#include "banjo/mcode/function.hpp"
#include "banjo/mcode/instruction.hpp"
#include "banjo/utils/generic_lexer.hpp"
#include "banjo/utils/hash_map.hpp"

#include <optional>

namespace banjo::mcode {

class Parser {

private:
    struct OperandContext {
        unsigned block_index;
        unsigned instr_index;
        unsigned operand_index;

        bool operator==(const OperandContext &other) const = default;
        bool operator!=(const OperandContext &other) const = default;
    };

protected:
    utils::TokenStream &tokens;

private:
    OperandContext operand_context;
    std::vector<std::pair<OperandContext, std::string>> unresolved_blocks;
    HashMap<std::string, BasicBlockIter> blocks_by_name;

public:
    Parser(utils::TokenStream &tokens);
    virtual ~Parser() = default;

    Function *parse_func();

private:
    std::optional<BasicBlock> parse_block(bool is_entry);
    std::optional<Instruction> parse_instr();

    bool resolve_idents(OperandContext context, Function &func);
    bool resolve_idents(OperandContext context, Instruction &instr);

protected:
    virtual std::optional<Opcode> parse_opcode() = 0;
    virtual std::optional<Operand> parse_operand() = 0;

    std::optional<Operand> parse_ident_operand();

    void report_unexpected(const std::string &expected);
    void report_unexpected();
    void report_error(const std::string &message);
};

} // namespace banjo::mcode

#endif
