#ifndef BANJO_TEST_UTIL_CODEGEN_UTIL_H
#define BANJO_TEST_UTIL_CODEGEN_UTIL_H

#include "banjo/target/target_description.hpp"

namespace banjo::test {

class CodegenUtil {

public:
    void lower(target::Architecture arch);
};

} // namespace banjo::test

#endif
