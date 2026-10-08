#ifndef BANJO_TARGET_AARCH64_PARSER_H
#define BANJO_TARGET_AARCH64_PARSER_H

#include "banjo/mcode/instruction.hpp"
#include "banjo/mcode/parser.hpp"
#include "banjo/mcode/register.hpp"
#include "banjo/target/aarch64/aarch64_address.hpp"

#include <optional>

namespace banjo::target {

class AArch64Parser final : public mcode::Parser {

public:
    using mcode::Parser::Parser;

private:
    std::optional<mcode::Opcode> parse_opcode();
    std::optional<mcode::Operand> parse_operand();

    std::optional<AArch64Address> parse_address();
    std::optional<mcode::Register> parse_register();
};

} // namespace banjo::target

#endif
