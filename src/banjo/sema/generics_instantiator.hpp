#ifndef GENERICS_INSTANTIATOR_H
#define GENERICS_INSTANTIATOR_H

#include "sema/semantic_analyzer_context.hpp"
#include "symbol/generics.hpp"

#include <optional>

namespace banjo {

namespace lang {

class GenericsInstantiator {

private:
    SemanticAnalyzerContext &context;

public:
    GenericsInstantiator(SemanticAnalyzerContext &context);
    GenericFuncInstance *instantiate_func(GenericFunc *generic_func, ASTNode *arg_list);
    GenericStructInstance *instantiate_struct(GenericStruct *generic_struct, ASTNode *arg_list);
    GenericFuncInstance *instantiate_func(GenericFunc *generic_func, GenericInstanceArgs &args);
    GenericStructInstance *instantiate_struct(GenericStruct *generic_struct, GenericInstanceArgs &args);

private:
    std::optional<GenericInstanceArgs> get_arg_list(ASTNode *node);

    template <typename T>
    GenericEntityInstance<T> *find_existing_instance(GenericInstanceArgs &args, GenericEntity<T> *generic_entity) {
        for (GenericEntityInstance<T> *existing_instance : generic_entity->get_instances()) {
            if (DataType::equal(existing_instance->args, args)) {
                return existing_instance;
            }
        }

        return nullptr;
    }
};

} // namespace lang

} // namespace banjo

#endif
