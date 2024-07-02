#include "method_table.hpp"

#include "banjo/symbol/function.hpp"

namespace banjo {

namespace lang {

Function *MethodTable::get_function(const std::string &name) {
    for (Function *function : functions) {
        if (function->get_name() == name) {
            return function;
        }
    }

    return nullptr;
}

std::vector<Function *> MethodTable::get_functions(const std::string &name) {
    std::vector<Function *> funcs;

    for (Function *func : functions) {
        if (func->get_name() == name) {
            funcs.push_back(func);
        }
    }

    return funcs;
}

} // namespace lang

} // namespace banjo
