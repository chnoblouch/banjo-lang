#include "closure_context.hpp"

namespace banjo::sema {

std::optional<unsigned> ClosureContext::try_capture(sir::Symbol symbol) {
    if (!symbol.is_one_of<sir::Local, sir::Param>()) {
        return {};
    }

    for (unsigned i = 0; i < captured_vars.size(); i++) {
        if (captured_vars[i] == symbol) {
            return i;
        }
    }

    unsigned index = captured_vars.size();
    captured_vars.push_back(symbol);
    return index;
}

} // namespace banjo::sema
