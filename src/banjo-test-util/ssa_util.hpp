#ifndef BANJO_TEST_UTIL_SSA_UTIL_H
#define BANJO_TEST_UTIL_SSA_UTIL_H

#include <string_view>

namespace banjo::test {

class SSAUtil {

public:
    void optimize(std::string_view pass_name, std::string_view source);
};

} // namespace banjo::test

#endif
