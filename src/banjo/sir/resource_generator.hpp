#ifndef BANJO_RESOURCE_GENERATOR_H
#define BANJO_RESOURCE_GENERATOR_H

#include "banjo/sir/sir.hpp"
#include "banjo/ssa_gen/specialization_collector.hpp"
#include "banjo/utils/arena.hpp"

#include <optional>

namespace banjo::sir {

class ResourceGenerator {

private:
    utils::Arena &arena;
    Ownership ownership;
    SpecializationCollector::Entry *specialization;

public:
    static bool is_resource(Expr type);

    ResourceGenerator(utils::Arena &arena);
    ResourceGenerator(utils::Arena &arena, Ownership ownership, SpecializationCollector::Entry &specialization);

    std::optional<Resource> create_resource(Expr type);
    std::optional<Resource> create_struct_resource(Concrete<StructDef> concrete_struct, Expr type);
    std::optional<Resource> create_tuple_resource(TupleExpr &tuple_type, Expr type);
    std::optional<Resource> create_generic_param_resource(const GenericParam &generic_param, Expr type);
};

} // namespace banjo::sir

#endif
