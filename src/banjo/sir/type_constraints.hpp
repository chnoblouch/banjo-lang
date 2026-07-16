#ifndef BANJO_SIR_TYPE_CONSTRAINTS_H
#define BANJO_SIR_TYPE_CONSTRAINTS_H

#include "banjo/sir/sir.hpp"
#include "banjo/sir/specializer.hpp"

#include <optional>

namespace banjo::lang::sir {

bool satisfies_type_constraint(TypeConstraint &constraint, Expr type, std::optional<Specializer> specializer = {});
bool satisfies_type_constraint_component(Expr component, Expr type, std::optional<Specializer> specializer);
bool implements(Expr type, Concrete<ProtoDef> concrete_proto);
bool contains(TypeConstraint &constraint, Concrete<ProtoDef> concrete_proto);
bool primitive_implements(Primitive primitive, Concrete<ProtoDef> concrete_proto);
bool pointer_implements(PointerType &pointer_type, Concrete<ProtoDef> concrete_proto);

} // namespace banjo::lang::sir

#endif
