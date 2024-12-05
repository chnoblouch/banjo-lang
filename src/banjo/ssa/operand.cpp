#include "operand.hpp"

#include "banjo/ssa/function.hpp"
#include "banjo/utils/macros.hpp"

namespace banjo {

namespace ssa {

const std::string &Operand::get_symbol_name() const {
    if (is_func()) return get_func()->get_name();
    else if (is_global()) return get_global_name();
    else if (is_extern_func()) return get_extern_func_name();
    else if (is_extern_global()) return get_extern_global_name();
    else ASSERT_UNREACHABLE;
}

} // namespace ssa

} // namespace banjo