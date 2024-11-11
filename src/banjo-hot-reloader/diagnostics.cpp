#include "diagnostics.hpp"

#include <cstdlib>
#include <iostream>

namespace banjo {

namespace hot_reloader {

namespace Diagnostics {

void log(const std::string &message) {
    std::cout << "(hot reloader) " << message << std::endl;
}

[[noreturn]] void abort(const std::string &message) {
    std::cerr << "(hot reloader) error: " << message << std::endl;
    std::exit(1);
}

std::string symbol_to_string(lang::sir::Symbol symbol) {
    std::string result;

    lang::sir::Symbol parent = symbol.get_parent();
    if (parent) {
        result += symbol_to_string(parent) + '.';
    }

    if (auto mod = symbol.match<lang::sir::Module>()) {
        result += mod->path.to_string();
    } else if (auto func_def = symbol.match<lang::sir::FuncDef>()) {
        result += func_def->ident.value;
    } else if (auto struct_def = symbol.match<lang::sir::StructDef>()) {
        result += struct_def->ident.value;
    } else if (auto union_def = symbol.match<lang::sir::UnionDef>()) {
        result += union_def->ident.value;
    }

    return result;
}

} // namespace Diagnostics

} // namespace hot_reloader

} // namespace banjo
