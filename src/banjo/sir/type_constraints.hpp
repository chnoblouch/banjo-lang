#ifndef BANJO_SIR_TYPE_CONSTRAINTS_H
#define BANJO_SIR_TYPE_CONSTRAINTS_H

#include "banjo/sir/sir.hpp"
#include "banjo/sir/specializer.hpp"

#include <optional>

namespace banjo::lang::sir {

bool satisfies_type_constraint(Expr constraint, Expr type, std::optional<Specializer> specializer = {});
bool primitive_implements(Primitive primitive, Concrete<ProtoDef> proto_def);

} // namespace banjo::lang::sir

#endif
