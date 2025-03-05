#ifndef BANJO_SSA_FUNCTION_TYPE_H
#define BANJO_SSA_FUNCTION_TYPE_H

#include "banjo/ssa/type.hpp"

#include <cstdint>
#include <vector>

namespace banjo {

namespace ssa {

enum class CallingConv : std::uint8_t {
    NONE,
    X86_64_SYS_V_ABI,
    X86_64_MS_ABI,
    AARCH64_AAPCS,
};

struct FunctionType {
    std::vector<Type> params;
    Type return_type;
    CallingConv calling_conv;
    bool variadic;
    unsigned first_variadic_index;
};

} // namespace ssa

} // namespace banjo

#endif
