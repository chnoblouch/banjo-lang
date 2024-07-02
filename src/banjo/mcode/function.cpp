#include "function.hpp"

#include <utility>

namespace banjo {

namespace mcode {

Function::Function(std::string name, CallingConvention *calling_conv)
  : name(std::move(name)),
    calling_conv(calling_conv) {}

} // namespace mcode

} // namespace banjo
