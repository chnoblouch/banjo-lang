#include "module.hpp"

#include <algorithm>

namespace banjo {

namespace ssa {

Module::~Module() {
    for (Function *func : functions) {
        delete func;
    }

    for (Structure *struct_ : structures) {
        delete struct_;
    }
}

Function *Module::get_function(const std::string &name) {
    auto predicate = [&name](const Function *func) { return func->get_name() == name; };
    auto it = std::find_if(functions.begin(), functions.end(), predicate);
    return it == functions.end() ? nullptr : *it;
}

Structure *Module::get_structure(const std::string &name) {
    auto predicate = [&name](const Structure *struct_) { return struct_->name == name; };
    auto it = std::find_if(structures.begin(), structures.end(), predicate);
    return it == structures.end() ? nullptr : *it;
}

void Module::forget_pointers() {
    functions.clear();
    structures.clear();
}

} // namespace ssa

} // namespace banjo
