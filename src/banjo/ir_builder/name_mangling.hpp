#ifndef IR_BUILDER_NAME_MANGLING_H
#define IR_BUILDER_NAME_MANGLING_H

#include "banjo/symbol/function.hpp"
#include "banjo/symbol/global_variable.hpp"
#include "banjo/symbol/module_path.hpp"
#include "banjo/symbol/structure.hpp"
#include "banjo/symbol/union.hpp"

#include <string>

namespace banjo {

namespace ir_builder {

namespace NameMangling {

std::string mangle_mod_path(const lang::ModulePath &mod_path);
std::string mangle_func_name(lang::Function *func);
std::string mangle_generic_func_name(lang::Function *func, const lang::GenericInstanceArgs &args);
std::string mangle_global_var_name(lang::GlobalVariable *global_var);
std::string mangle_type_name(lang::DataType *type);
std::string mangle_struct_name(lang::Structure *struct_);
std::string mangle_union_name(lang::Union *union_);
std::string mangle_union_case_name(lang::UnionCase *case_);
std::string mangle_proto_name(lang::Protocol *proto);
std::string mangle_generic_struct_name(lang::Structure *struct_, const lang::GenericInstanceArgs &instance);

} // namespace NameMangling

} // namespace ir_builder

} // namespace banjo

#endif
