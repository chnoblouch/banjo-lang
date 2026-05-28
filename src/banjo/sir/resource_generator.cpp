#include "resource_generator.hpp"

#include "banjo/sir/magic_methods.hpp"
#include "banjo/sir/specializer.hpp"
#include "banjo/utils/arena.hpp"

namespace banjo::lang::sir {

ResourceGenerator::ResourceGenerator(utils::Arena &arena)
  : arena{arena},
    ownership{Ownership::OWNED},
    specialization{nullptr} {}

ResourceGenerator::ResourceGenerator(
    utils::Arena &arena,
    sir::Ownership ownership,
    SpecializationCollector::Entry &specialization
)
  : arena{arena},
    ownership{ownership},
    specialization{&specialization} {}

std::optional<sir::Resource> ResourceGenerator::create_resource(sir::Expr type) {
    if (auto concrete_struct = type.match_concrete<sir::StructDef>()) {
        return create_struct_resource(*concrete_struct, type);
    } else if (auto tuple_type = type.match<sir::TupleExpr>()) {
        return create_tuple_resource(*tuple_type, type);
    } else if (auto closure_type = type.match<sir::ClosureType>()) {
        return create_struct_resource({closure_type->underlying_struct}, type);
    } else if (auto generic_param = type.match_symbol<sir::GenericParam>()) {
        return create_generic_param_resource(*generic_param, type);
    } else {
        return {};
    }
}

std::optional<sir::Resource> ResourceGenerator::create_struct_resource(
    sir::Concrete<sir::StructDef> concrete_struct,
    sir::Expr type
) {
    if (specialization) {
        sir::Specializer specializer{arena, specialization->params, specialization->args};
        concrete_struct.generic_args = specializer.specialize_expr_list(concrete_struct.generic_args);
        type = specializer.specialize_expr(type);
    }

    std::optional<sir::Resource> resource;

    sir::SymbolTable &symbol_table = *concrete_struct.def->block.symbol_table;
    auto iter = symbol_table.symbols.find(sir::MagicMethods::DEINIT);

    if (iter != symbol_table.symbols.end() && iter->second.is<sir::FuncDef>()) {
        resource = sir::Resource{
            .type = type,
            .has_deinit = true,
            .ownership = ownership,
            .sub_resources{},
        };
    }

    for (unsigned i = 0; i < concrete_struct.def->fields.size(); i++) {
        sir::StructField &field = *concrete_struct.def->fields[i];
        if (field.attrs && field.attrs->unmanaged) {
            continue;
        }

        sir::Expr field_type = field.type;

        if (concrete_struct.is_specialization()) {
            field_type = sir::Specializer{arena, concrete_struct}.specialize_expr(field_type);
        }

        std::optional<sir::Resource> sub_resource = create_resource(field_type);
        if (!sub_resource) {
            continue;
        }

        if (!resource) {
            resource = sir::Resource{
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

std::optional<sir::Resource> ResourceGenerator::create_tuple_resource(sir::TupleExpr &tuple_type, sir::Expr type) {
    std::optional<sir::Resource> resource;

    for (unsigned i = 0; i < tuple_type.exprs.size(); i++) {
        std::optional<sir::Resource> sub_resource = create_resource(tuple_type.exprs[i]);
        if (!sub_resource) {
            continue;
        }

        if (!resource) {
            resource = sir::Resource{
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

std::optional<sir::Resource> ResourceGenerator::create_generic_param_resource(
    const sir::GenericParam &generic_param,
    sir::Expr type
) {
    if (specialization) {
        sir::Expr resolved_type = specialization->resolve_param(generic_param);
        return create_resource(resolved_type);
    } else {
        return sir::Resource{
            .type = type,
            .has_deinit = false,
            .ownership = ownership,
            .sub_resources{},
        };
    }
}

} // namespace banjo::lang::sir
