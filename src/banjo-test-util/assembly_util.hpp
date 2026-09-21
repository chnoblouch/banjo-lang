#ifndef BANJO_TEST_UTIL_ASSEMBLY_UTIL_H
#define BANJO_TEST_UTIL_ASSEMBLY_UTIL_H

#include "banjo/mcode/instruction.hpp"
#include "banjo/target/target_description.hpp"
#include "banjo/utils/write_buffer.hpp"
#include "line_based_reader.hpp"

#include <optional>
#include <string>

namespace banjo::test {

class AssemblyUtil {

private:
    target::Architecture arch;
    LineBasedReader reader;

public:
    AssemblyUtil(target::Architecture arch);
    WriteBuffer assemble();

private:
    std::optional<mcode::Instruction> parse_line();
    mcode::Opcode parse_opcode();
    mcode::Operand parse_operand();

    mcode::Opcode convert_opcode(const std::string &string);
    mcode::Register convert_register(const std::string &string);

    std::string read_operand();
};

} // namespace banjo::test

#endif
