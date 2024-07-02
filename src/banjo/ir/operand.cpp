#include "operand.hpp"

#include "ir/function.hpp"
#include "utils/macros.hpp"

namespace banjo {

namespace ir {

const std::string &Operand::get_symbol_name() const {
    if (is_func()) return get_func()->get_name();
    else if (is_global()) return get_global_name();
    else if (is_extern_func()) return get_extern_func_name();
    else if (is_extern_global()) return get_extern_global_name();
    else ASSERT_UNREACHABLE;
}

} // namespace ir

} // namespace banjo
