#ifndef GENERICS_ARG_DEDUCTION_H
#define GENERICS_ARG_DEDUCTION_H

#include "ast/ast_node.hpp"
#include "symbol/data_type_manager.hpp"
#include "symbol/generics.hpp"

#include <optional>

namespace banjo {

namespace lang {

class GenericsArgDeduction {

private:
    DataTypeManager &type_manager;
    const std::vector<GenericParam> &generic_params;
    bool has_sequence;
    std::vector<DataType *> generic_args;

public:
    GenericsArgDeduction(
        DataTypeManager &type_manager,
        const std::vector<GenericParam> &generic_params,
        bool has_sequence
    );

    std::optional<std::vector<DataType *>> deduce(
        const std::vector<ASTNode *> &param_nodes,
        const std::vector<DataType *> &arg_types
    );

private:
    void deduce(ASTNode *param_type, DataType *arg_type);
    void store_deduced_type(const std::string &param_name, DataType *type);
};

} // namespace lang

} // namespace banjo

#endif
