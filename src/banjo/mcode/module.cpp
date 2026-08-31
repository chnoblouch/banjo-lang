#include "module.hpp"

#include "banjo/mcode/function.hpp"

namespace banjo {

namespace mcode {

Module::~Module() {
    for (Function *func : functions) {
        delete func;
    }
}

} // namespace mcode

} // namespace banjo
