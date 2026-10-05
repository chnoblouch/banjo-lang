#ifndef BANJO_TEST_UTIL_CODEGEN_UTIL_H
#define BANJO_TEST_UTIL_CODEGEN_UTIL_H

#include "banjo/target/target_description.hpp"

#include <string_view>

namespace banjo::test {

class CodegenUtil {

public:
    void run(target::Architecture arch, std::string_view source);
};

} // namespace banjo::test

#endif
