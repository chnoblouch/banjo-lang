#include "resource_generator.hpp"

#include "banjo/sir/magic_methods.hpp"
#include "banjo/sir/sir.hpp"
#include "banjo/sir/specializer.hpp"
#include "banjo/utils/arena.hpp"

namespace banjo::sir {

bool ResourceGenerator::is_resource(Expr type) {
    // TODO: Optimize
    utils::Arena arena;
    return ResourceGenerator{arena}.create_resource(type).has_value();
}

ResourceGenerator::ResourceGenerator(utils::Arena &arena)
  : arena{arena},
    ownership{Ownership::OWNED},
    specialization{nullptr} {}

ResourceGenerator::ResourceGenerator(
    utils::Arena &arena,
    Ownership ownership,
    SpecializationCollector::Entry &specialization
)
  : arena{arena},
    ownership{ownership},
    specialization{&specialization} {}

std::optional<Resource> ResourceGenerator::create_resource(Expr type) {
    if (auto concrete_struct = type.match_concrete<StructDef>()) {
        return create_struct_resource(*concrete_struct, type);
    } else if (auto tuple_type = type.match<TupleExpr>()) {
        return create_tuple_resource(*tuple_type, type);
    } else if (auto closure_type = type.match<ClosureType>()) {
        return create_struct_resource({closure_type->underlying_struct}, type);
    } else if (auto generic_param = type.match_symbol<GenericParam>()) {
        return create_generic_param_resource(*generic_param, type);
    } else {
        return {};
    }
}

std::optional<Resource> ResourceGenerator::create_struct_resource(Concrete<StructDef> concrete_struct, Expr type) {
    if (specialization) {
        Specializer specializer{arena, specialization->params, specialization->args};
        concrete_struct.generic_args = specializer.specialize_expr_list(concrete_struct.generic_args);
        type = specializer.specialize_expr(type);
    }

    std::optional<Resource> resource;

    SymbolTable &symbol_table = *concrete_struct.def->block.symbol_table;
    auto iter = symbol_table.symbols.find(MagicMethods::DEINIT);

    if (iter != symbol_table.symbols.end() && iter->second.is<FuncDef>()) {
        resource = Resource{
            .type = type,
            .has_deinit = true,
            .ownership = ownership,
            .sub_resources{},
        };
    }

    for (unsigned i = 0; i < concrete_struct.def->fields.size(); i++) {
        StructField &field = *concrete_struct.def->fields[i];
        if (field.attrs && field.attrs->unmanaged) {
            continue;
        }

        Expr field_type = field.type;

        if (concrete_struct.is_specialization()) {
            field_type = Specializer{arena, concrete_struct}.specialize_expr(field_type);
        }

        std::optional<Resource> sub_resource = create_resource(field_type);
        if (!sub_resource) {
            continue;
        }

        if (!resource) {
            resource = Resource{
                .type = type,
                .has_deinit = false,
                .ownership = ownership,
                .sub_resources{},
            };
        }

        sub_resource->field_index = i;
        resource->sub_resources.push_back(*sub_resource);
    }

    return resource;
}

std::optional<Resource> ResourceGenerator::create_tuple_resource(TupleExpr &tuple_type, Expr type) {
    std::optional<Resource> resource;

    for (unsigned i = 0; i < tuple_type.exprs.size(); i++) {
        std::optional<Resource> sub_resource = create_resource(tuple_type.exprs[i]);
        if (!sub_resource) {
            continue;
        }

        if (!resource) {
            resource = Resource{
                .type = type,
                .has_deinit = false,
                .ownership = ownership,
                .sub_resources{},
            };
        }

        sub_resource->field_index = i;
        resource->sub_resources.push_back(*sub_resource);
    }

    return resource;
}

std::optional<Resource> ResourceGenerator::create_generic_param_resource(const GenericParam &generic_param, Expr type) {
    for (Expr component : generic_param.constraint.components) {
        if (auto concrete_proto = component.match_concrete<ProtoDef>()) {
            if (concrete_proto->def->role == ProtoDef::Role::COPY) {
                return {};
            }
        }
    }

    if (specialization) {
        Expr resolved_type = specialization->resolve_param(generic_param);
        return create_resource(resolved_type);
    } else {
        return Resource{
            .type = type,
            .has_deinit = false,
            .ownership = ownership,
            .sub_resources{},
        };
    }
}

} // namespace banjo::sir
