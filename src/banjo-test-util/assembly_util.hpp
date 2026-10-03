#ifndef BANJO_TEST_UTIL_ASSEMBLY_UTIL_H
#define BANJO_TEST_UTIL_ASSEMBLY_UTIL_H

#include "assembler_lexer.hpp"
#include "banjo/target/target_description.hpp"
#include "banjo/utils/write_buffer.hpp"

#include <string_view>

namespace banjo::test {

class AssemblyUtil {

private:
    target::Architecture arch;
    std::string_view source;
    assembler::TokenStream tokens;

public:
    AssemblyUtil(target::Architecture arch, std::string_view source);
    WriteBuffer assemble();
};

} // namespace banjo::test

#endif
