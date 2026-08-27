#ifndef BANJO_SIR_TYPE_CONSTRAINTS_H
#define BANJO_SIR_TYPE_CONSTRAINTS_H

#include "banjo/sir/sir.hpp"
#include "banjo/sir/specializer.hpp"

namespace banjo::sir {

bool satisfies_type_constraint(TypeConstraint &constraint, Expr type, Specializer *specializer = nullptr);
bool satisfies_type_constraint_component(Expr component, Expr type, Specializer *specializer);
bool implements(Expr type, Concrete<ProtoDef> concrete_proto);
bool implements(TypeConstraint &constraint, Concrete<ProtoDef> concrete_proto);
bool primitive_implements(Primitive primitive, Concrete<ProtoDef> concrete_proto);
bool pointer_implements(PointerType &pointer_type, Concrete<ProtoDef> concrete_proto);
bool is_superset(TypeConstraint &a, TypeConstraint &b, Specializer *specializer);

} // namespace banjo::sir

#endif
