#ifndef NAME_MANGLING_H
#define NAME_MANGLING_H

#include "banjo/sir/sir.hpp"
#include "banjo/ssa_gen/ssa_generator_context.hpp"

namespace banjo {

namespace lang {

namespace NameMangling {

std::string mangle_func_name(SSAGeneratorContext &ctx, const sir::FuncDef &func);

};

} // namespace lang

} // namespace banjo

#endif
