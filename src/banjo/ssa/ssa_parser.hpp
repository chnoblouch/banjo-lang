#ifndef BANJO_SSA_PARSER_H
#define BANJO_SSA_PARSER_H

#include "banjo/ssa/basic_block.hpp"
#include "banjo/ssa/function.hpp"
#include "banjo/ssa/function_type.hpp"
#include "banjo/ssa/module.hpp"
#include "banjo/ssa/opcode.hpp"
#include "banjo/ssa/operand.hpp"
#include "banjo/ssa/structure.hpp"
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

    HashMap<std::string, Structure *> structs_by_name;
    HashMap<std::string, BasicBlockIter> blocks_by_name;
    HashMap<std::string, Function *> funcs_by_name;
    HashMap<std::string, FunctionDecl *> extern_funcs_by_name;

public:
    Parser(utils::TokenStream &tokens, CallingConv calling_conv);

    Module parse();

private:
    bool parse_func(Module &mod);
    std::optional<std::vector<Param>> parse_params();
    std::optional<BasicBlock> parse_block(bool is_entry);
    std::optional<Instruction> parse_instr();
    std::optional<Opcode> parse_opcode();
    std::optional<Operand> parse_operand();
    std::optional<VirtualRegister> parse_reg();

    bool parse_struct(Module &mod);

    std::optional<Type> parse_type();
    std::optional<std::string> parse_ident();
    bool try_end_line();

    bool resolve_idents(OperandContext context, Function &func);
    bool resolve_idents(OperandContext context, Instruction &instr);

    void report_unexpected(const std::string &expected);
    void report_unexpected();
    void report_error(const std::string &message);
};

} // namespace banjo::ssa

#endif
