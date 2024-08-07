#include "module.hpp"

#include "banjo/mcode/function.hpp"

namespace banjo {

namespace mcode {

Module::~Module() {
    for (Function *func : functions) {
        delete func;
    }
}

std::string Module::next_float_label() {
    return "float." + std::to_string(last_float_label_id++);
}

} // namespace mcode

} // namespace banjo
