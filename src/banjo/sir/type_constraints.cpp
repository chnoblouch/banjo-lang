#include "type_constraints.hpp"

#include "banjo/sir/resource_generator.hpp"
#include "banjo/sir/sir.hpp"
#include "banjo/sir/sir_comparison.hpp"
#include "banjo/utils/arena.hpp"
#include "banjo/utils/macros.hpp"
#include "banjo/utils/utils.hpp"

#include <initializer_list>

namespace banjo::sir {

bool satisfies_type_constraint(
    TypeConstraint &constraint,
    Expr type,
    Specializer *specializer /* = nullptr */
) {
    if (constraint.kind == TypeConstraint::Kind::INTERSECTION) {
        for (Expr component : constraint.components) {
            if (!satisfies_type_constraint_component(component, type, specializer)) {
                return false;
            }
        }

        return true;
    } else if (constraint.kind == TypeConstraint::Kind::UNION) {
        if (auto generic_param = type.match_symbol<sir::GenericParam>()) {
            return is_superset(constraint, generic_param->constraint, specializer);
        }

        for (Expr component : constraint.components) {
            if (satisfies_type_constraint_component(component, type, specializer)) {
                return true;
            }
        }

        return false;
    } else {
        ASSERT_UNREACHABLE;
    }
}

bool satisfies_type_constraint_component(Expr component, Expr type, Specializer *specializer) {
    if (auto concrete_proto = component.match_concrete<ProtoDef>()) {
        if (specializer) {
            concrete_proto->generic_args = specializer->specialize_expr_list(concrete_proto->generic_args);
        }

        return implements(type, *concrete_proto);
    } else {
        return type == component;
    }
}

bool implements(Expr type, Concrete<ProtoDef> concrete_proto) {
    bool satisfied = false;

    if (auto primitive_type = type.match<PrimitiveType>()) {
        satisfied = primitive_implements(primitive_type->primitive, concrete_proto);
    } else if (auto pointer_type = type.match<PointerType>()) {
        satisfied = pointer_implements(*pointer_type, concrete_proto);
    } else if (auto concrete_struct = type.match_concrete<StructDef>()) {
        satisfied = concrete_struct->def->has_impl_for(concrete_proto);
    } else if (auto param = type.match_symbol<GenericParam>()) {
        satisfied = implements(param->constraint, concrete_proto);
    }

    if (!satisfied && concrete_proto.def->role == ProtoDef::Role::COPY) {
        utils::Arena arena;
        satisfied = !ResourceGenerator{arena}.create_resource(type).has_value();
    }

    return satisfied;
}

bool implements(TypeConstraint &constraint, Concrete<ProtoDef> concrete_proto) {
    for (sir::Expr component : constraint.components) {
        if (auto other_proto = component.match_concrete<sir::ProtoDef>()) {
            if (other_proto == concrete_proto) {
                return true;
            }
        }
    }

    return false;
}

bool primitive_implements(Primitive primitive, Concrete<ProtoDef> concrete_proto) {
    std::initializer_list<ProtoDef::Role> numeric_protos{
        ProtoDef::Role::ORDER,
        ProtoDef::Role::ADD,
        ProtoDef::Role::SUB,
        ProtoDef::Role::MUL,
        ProtoDef::Role::DIV,
    };

    std::initializer_list<ProtoDef::Role> int_protos{
        ProtoDef::Role::MOD,
        ProtoDef::Role::BIT_AND,
        ProtoDef::Role::BIT_OR,
        ProtoDef::Role::BIT_XOR,
        ProtoDef::Role::SHL,
        ProtoDef::Role::SHR,
    };

    if (concrete_proto.def->role == ProtoDef::Role::COMPARE) {
        if (primitive == Primitive::VOID) {
            return false;
        }

        return concrete_proto.generic_args[0].is_primitive_type(primitive);
    } else if (utils::is_one_of(concrete_proto.def->role, numeric_protos)) {
        switch (primitive) {
            case Primitive::I8:
            case Primitive::I16:
            case Primitive::I32:
            case Primitive::I64:
            case Primitive::U8:
            case Primitive::U16:
            case Primitive::U32:
            case Primitive::U64:
            case Primitive::USIZE:
            case Primitive::F32:
            case Primitive::F64: return concrete_proto.generic_args[0].is_primitive_type(primitive);
            default: return false;
        }
    } else if (utils::is_one_of(concrete_proto.def->role, int_protos)) {
        switch (primitive) {
            case Primitive::I8:
            case Primitive::I16:
            case Primitive::I32:
            case Primitive::I64:
            case Primitive::U8:
            case Primitive::U16:
            case Primitive::U32:
            case Primitive::U64:
            case Primitive::USIZE: return concrete_proto.generic_args[0].is_primitive_type(primitive);
            default: return false;
        }
    } else if (concrete_proto.def->role == ProtoDef::Role::BIT_NOT) {
        switch (primitive) {
            case Primitive::I8:
            case Primitive::I16:
            case Primitive::I32:
            case Primitive::I64:
            case Primitive::U8:
            case Primitive::U16:
            case Primitive::U32:
            case Primitive::U64:
            case Primitive::USIZE: return true;
            default: return false;
        }
    } else if (concrete_proto.def->role == ProtoDef::Role::NEG) {
        switch (primitive) {
            case Primitive::I8:
            case Primitive::I16:
            case Primitive::I32:
            case Primitive::I64: return true;
            default: return false;
        }
    } else {
        return false;
    }
}

bool pointer_implements(PointerType &pointer_type, Concrete<ProtoDef> concrete_proto) {
    // TODO: addition, subtraction, comparisons against address-like types.

    if (pointer_type.base_type.is_symbol<sir::ProtoDef>()) {
        return false;
    }

    if (concrete_proto.def->role == ProtoDef::Role::COMPARE) {
        return concrete_proto.generic_args[0] == &pointer_type;
    } else {
        return false;
    }
}

bool is_superset(TypeConstraint &a, TypeConstraint &b, Specializer *specializer) {
    if (a.kind != TypeConstraint::Kind::UNION || b.kind != TypeConstraint::Kind::UNION) {
        return false;
    }

    Comparison comparison{[&](Comparison &self, Expr lhs, Expr rhs) -> std::optional<bool> {
        auto lhs_specialize_expr = lhs.match<sir::SpecializeExpr>();
        auto rhs_specialize_expr = rhs.match<sir::SpecializeExpr>();

        if (lhs_specialize_expr && rhs_specialize_expr) {
            utils::Arena arena;

            if (lhs_specialize_expr->symbol != rhs_specialize_expr->symbol) {
                return false;
            }

            std::span<Expr> lhs_args = specializer->specialize_expr_list(lhs_specialize_expr->args);
            std::span<Expr> rhs_args = specializer->specialize_expr_list(rhs_specialize_expr->args);

            return self.compare(lhs_args, rhs_args);
        } else {
            return {};
        }
    }};

    for (Expr b_component : b.components) {
        bool found = false;

        for (Expr a_component : a.components) {
            if (comparison.compare(a_component, b_component)) {
                found = true;
                break;
            }
        }

        if (!found) {
            return false;
        }
    }

    return true;
}

} // namespace banjo::sir
