#ifndef BANJO_TEST_UTIL_ASSEMBLY_UTIL_H
#define BANJO_TEST_UTIL_ASSEMBLY_UTIL_H

#include "banjo/target/target_description.hpp"
#include "banjo/utils/write_buffer.hpp"

#include <string_view>

namespace banjo::test {

class AssemblyUtil {

private:
    target::Architecture arch;

public:
    AssemblyUtil(target::Architecture arch);
    WriteBuffer assemble(std::string_view source);
};

} // namespace banjo::test

#endif
