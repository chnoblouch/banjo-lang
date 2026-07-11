#include "type_constraints.hpp"

#include "banjo/sir/resource_generator.hpp"
#include "banjo/sir/sir.hpp"
#include "banjo/utils/arena.hpp"
#include "banjo/utils/utils.hpp"

#include <initializer_list>

namespace banjo::lang::sir {

bool satisfies_type_constraint(
    TypeConstraint &constraint,
    Expr type,
    std::optional<Specializer> specializer /* = {} */
) {
    for (Expr component : constraint.components) {
        if (!satisfies_type_constraint_component(component, type, specializer)) {
            return false;
        }
    }

    return true;
}

bool satisfies_type_constraint_component(Expr component, Expr type, std::optional<Specializer> specializer) {
    if (!component.match_concrete<ProtoDef>()) {
        return type == component;
    }

    Concrete<ProtoDef> concrete_proto = component.as_concrete<ProtoDef>();

    if (specializer) {
        concrete_proto.generic_args = specializer->specialize_expr_list(concrete_proto.generic_args);
    }

    bool satisfied = false;

    if (auto primitive_type = type.match<PrimitiveType>()) {
        satisfied = primitive_implements(primitive_type->primitive, concrete_proto);
    } else if (auto pointer_type = type.match<PointerType>()) {
        satisfied = pointer_implements(*pointer_type, concrete_proto);
    } else if (auto concrete_struct = type.match_concrete<StructDef>()) {
        satisfied = concrete_struct->def->has_impl_for(concrete_proto);
    } else if (auto param = type.match_symbol<GenericParam>()) {
        satisfied = contains(param->constraint, concrete_proto);
    }

    if (!satisfied && concrete_proto.def->role == ProtoDef::Role::COPY) {
        utils::Arena arena;
        satisfied = !ResourceGenerator{arena}.create_resource(type).has_value();
    }

    return satisfied;
}

bool contains(TypeConstraint &constraint, Concrete<ProtoDef> proto_def) {
    for (sir::Expr component : constraint.components) {
        if (auto other_proto = component.match_concrete<sir::ProtoDef>()) {
            if (other_proto == proto_def) {
                return true;
            }
        }
    }

    return false;
}

bool primitive_implements(Primitive primitive, Concrete<ProtoDef> proto_def) {
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

    if (proto_def.def->role == ProtoDef::Role::COMPARE) {
        if (primitive == Primitive::VOID) {
            return false;
        }

        return proto_def.generic_args[0].is_primitive_type(primitive);
    } else if (Utils::is_one_of(proto_def.def->role, numeric_protos)) {
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
            case Primitive::F64: return proto_def.generic_args[0].is_primitive_type(primitive);
            case Primitive::BOOL:
            case Primitive::ADDR:
            case Primitive::VOID: return false;
        }
    } else if (Utils::is_one_of(proto_def.def->role, int_protos)) {
        switch (primitive) {
            case Primitive::I8:
            case Primitive::I16:
            case Primitive::I32:
            case Primitive::I64:
            case Primitive::U8:
            case Primitive::U16:
            case Primitive::U32:
            case Primitive::U64:
            case Primitive::USIZE: return proto_def.generic_args[0].is_primitive_type(primitive);
            case Primitive::F32:
            case Primitive::F64:
            case Primitive::BOOL:
            case Primitive::ADDR:
            case Primitive::VOID: return false;
        }
    } else {
        return false;
    }
}

bool pointer_implements(PointerType &pointer_type, Concrete<ProtoDef> proto_def) {
    // TODO: addition, subtraction, comparisons against address-like types.

    if (pointer_type.base_type.is_symbol<sir::ProtoDef>()) {
        return false;
    }

    if (proto_def.def->role == ProtoDef::Role::COMPARE) {
        return proto_def.generic_args[0] == &pointer_type;
    } else {
        return false;
    }
}

} // namespace banjo::lang::sir
