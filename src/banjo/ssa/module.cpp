#include "module.hpp"

#include <algorithm>

namespace banjo {

namespace ssa {

Module::~Module() {
    for (Function *func : functions) {
        delete func;
    }

    for (Global *global : globals) {
        delete global;
    }

    for (Structure *struct_ : structures) {
        delete struct_;
    }

    for (FunctionDecl *func : external_functions) {
        delete func;
    }

    for (GlobalDecl *global : external_globals) {
        delete global;
    }
}

Function *Module::get_function(const std::string &name) {
    auto predicate = [&name](const Function *func) { return func->name == name; };
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
    globals.clear();
    structures.clear();
    external_functions.clear();
    external_globals.clear();
}

} // namespace ssa

} // namespace banjo
