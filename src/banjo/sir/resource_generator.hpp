#ifndef BANJO_RESOURCE_GENERATOR_H
#define BANJO_RESOURCE_GENERATOR_H

#include "banjo/sir/sir.hpp"
#include "banjo/ssa_gen/specialization_collector.hpp"
#include "banjo/utils/arena.hpp"

#include <optional>

namespace banjo::lang::sir {

class ResourceGenerator {

private:
    utils::Arena &arena;
    sir::Ownership ownership;
    SpecializationCollector::Entry *specialization;

public:
    ResourceGenerator(utils::Arena &arena);
    ResourceGenerator(utils::Arena &arena, sir::Ownership ownership, SpecializationCollector::Entry &specialization);

    std::optional<sir::Resource> create_resource(sir::Expr type);
    std::optional<sir::Resource> create_struct_resource(sir::Concrete<sir::StructDef> concrete_struct, sir::Expr type);
    std::optional<sir::Resource> create_tuple_resource(sir::TupleExpr &tuple_type, sir::Expr type);
    std::optional<sir::Resource> create_generic_param_resource(const sir::GenericParam &generic_param, sir::Expr type);
};

} // namespace banjo::lang::sir

#endif
