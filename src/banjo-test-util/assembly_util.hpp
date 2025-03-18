#ifndef BANJO_TEST_UTIL_ASSEMBLY_UTIL_H
#define BANJO_TEST_UTIL_ASSEMBLY_UTIL_H

#include "banjo/mcode/instruction.hpp"
#include "banjo/utils/write_buffer.hpp"

#include <optional>
#include <string>

namespace banjo {
namespace test {

class AssemblyUtil {

private:
    std::string line;
    unsigned char_index;

public:
    WriteBuffer assemble(std::string source);

private:
    std::optional<mcode::Instruction> parse_line();
    mcode::Opcode parse_opcode();
    mcode::Operand parse_operand();

    mcode::Opcode convert_opcode(const std::string &string);
    void skip_whitespace();

    bool is_whitespace(char c);
};

} // namespace test
} // namespace banjo

#endif
