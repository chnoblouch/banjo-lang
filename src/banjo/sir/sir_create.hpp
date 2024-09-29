#ifndef SIR_CREATE_H
#define SIR_CREATE_H

#include "banjo/sir/sir.hpp"

namespace banjo {

namespace lang {

namespace sir {

sir::Expr create_tuple_value(sir::Module &mod, std::vector<sir::Expr> values);
sir::Expr create_primitive_type(sir::Module &mod, sir::Primitive primitive);
sir::Expr create_call(sir::Module &mod, sir::FuncDef &func_def, std::vector<sir::Expr> args);
sir::Expr create_unary_ref(sir::Module &mod, sir::Expr base_value);

} // namespace sir

} // namespace lang

} // namespace banjo

#endif
