#ifndef BANJO_TEST_UTIL_SSA_PARSER_H
#define BANJO_TEST_UTIL_SSA_PARSER_H

#include "banjo/ssa/function_type.hpp"
#include "banjo/ssa/module.hpp"
#include "banjo/ssa/type.hpp"
#include "banjo/ssa/virtual_register.hpp"
#include "line_based_reader.hpp"

#include <string>
#include <vector>

namespace banjo {
namespace test {

class SSAParser {

private:
    LineBasedReader reader;

    ssa::Module mod;
    std::unordered_map<std::string_view, ssa::Function *> funcs;
    std::unordered_map<std::string_view, ssa::BasicBlockIter> blocks;

public:
    SSAParser();
    ssa::Module parse();

private:
    std::string parse_identifier();
    ssa::Type parse_type();
    std::vector<ssa::Type> parse_params();
    ssa::CallingConv parse_calling_conv();

    ssa::Instruction parse_instr(std::optional<ssa::VirtualRegister> dst);
    ssa::Opcode parse_op();
    std::vector<ssa::Operand> parse_operands();
    std::optional<ssa::Operand> parse_operand();
};

} // namespace test
} // namespace banjo

#endif
