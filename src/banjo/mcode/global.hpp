#ifndef BANJO_MCODE_GLOBAL_H
#define BANJO_MCODE_GLOBAL_H

#include "banjo/utils/large_int.hpp"

#include <cstdint>
#include <string>
#include <variant>
#include <vector>

namespace banjo {

namespace mcode {

struct Global {
    typedef std::monostate None;
    typedef LargeInt Integer;
    typedef double FloatingPoint;
    typedef std::vector<std::uint8_t> Bytes;
    typedef std::string String;

    struct SymbolRef {
        std::string name;
    };

    typedef std::variant<None, Integer, FloatingPoint, Bytes, String, SymbolRef> Value;

    std::string name;
    unsigned size;
    unsigned alignment = 0;
    Value value;
};

} // namespace mcode

} // namespace banjo

#endif
