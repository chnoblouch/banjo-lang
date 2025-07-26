#ifndef BANJO_SSA_GLOBAL_H
#define BANJO_SSA_GLOBAL_H

#include "banjo/ssa/type.hpp"
#include "banjo/utils/large_int.hpp"

#include <cstdint>
#include <string>
#include <variant>
#include <vector>

namespace banjo {

namespace ssa {

class Function;

struct GlobalDecl {
    std::string name;
    Type type;
};

struct Global {
    typedef std::monostate None;
    typedef LargeInt Integer;
    typedef double FloatingPoint;
    typedef std::vector<std::uint8_t> Bytes;
    typedef std::string String;

    struct GlobalRef {
        std::string name;
    };

    struct ExternFuncRef {
        std::string name;
    };

    struct ExternGlobalRef {
        std::string name;
    };

    typedef std::
        variant<None, Integer, FloatingPoint, Bytes, String, Function *, GlobalRef, ExternFuncRef, ExternGlobalRef>
            Value;

    std::string name;
    Type type;
    Value initial_value;
    bool external;
};

} // namespace ssa

} // namespace banjo

#endif
