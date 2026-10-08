#ifndef BANJO_TARGET_X86_64_PARSER_H
#define BANJO_TARGET_X86_64_PARSER_H

#include "banjo/mcode/instruction.hpp"
#include "banjo/mcode/parser.hpp"
#include "banjo/target/x86_64/x86_64_address.hpp"

#include <optional>
#include <utility>

namespace banjo::target {

class X8664Parser final : public mcode::Parser {

public:
    using mcode::Parser::Parser;

private:
    std::optional<mcode::Opcode> parse_opcode() override;
    std::optional<mcode::Operand> parse_operand() override;

    std::optional<std::pair<X8664Address, unsigned>> parse_address();
    std::optional<std::pair<mcode::Register, unsigned>> parse_register();
};

} // namespace banjo::target

#endif
