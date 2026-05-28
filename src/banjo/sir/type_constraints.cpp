#include "type_constraints.hpp"

#include "banjo/sir/resource_generator.hpp"
#include "banjo/sir/sir.hpp"
#include "banjo/utils/arena.hpp"
#include "banjo/utils/utils.hpp"

#include <initializer_list>

namespace banjo::lang::sir {

bool satisfies_type_constraint(
    Expr constraint,
    Expr type,
    std::optional<Specializer> specializer /* = {} */
) {
    if (!constraint.match_concrete<ProtoDef>()) {
        return type == constraint;
    }

    Concrete<ProtoDef> concrete_proto = constraint.as_concrete<ProtoDef>();

    if (specializer) {
        concrete_proto.generic_args = specializer->specialize_expr_list(concrete_proto.generic_args);
    }

    bool satisfied = false;

    if (auto primitive_type = type.match<PrimitiveType>()) {
        satisfied = primitive_implements(primitive_type->primitive, concrete_proto);
    } else if (auto concrete_struct = type.match_concrete<StructDef>()) {
        satisfied = concrete_struct->def->has_impl_for(concrete_proto);
    } else if (auto param = type.match_symbol<GenericParam>()) {
        if (auto other_proto = param->constraint.match_concrete<ProtoDef>()) {
            satisfied = concrete_proto == other_proto;
        }
    }

    if (!satisfied && concrete_proto.def->role == ProtoDef::Role::COPY) {
        utils::Arena arena;
        satisfied = !ResourceGenerator{arena}.create_resource(type).has_value();
    }

    return satisfied;
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

} // namespace banjo::lang::sir
