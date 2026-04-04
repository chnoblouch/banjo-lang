#ifndef BANJO_SSA_GENERATOR_NAME_MANGLING_H
#define BANJO_SSA_GENERATOR_NAME_MANGLING_H

#include "banjo/sir/sir.hpp"

#include <vector>

namespace banjo {

namespace lang {

namespace NameMangling {

std::string get_link_name(const sir::FuncDef &func, const std::vector<sir::Expr> &generic_args);
std::string get_link_name(const sir::NativeFuncDecl &func);

std::string mangle_func_name(const sir::FuncDef &func, const std::vector<sir::Expr> &generic_args);

}; // namespace NameMangling

} // namespace lang

} // namespace banjo

#endif
