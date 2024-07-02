#include "generics_arg_deduction.hpp"

#include "ast/ast_child_indices.hpp"
#include "ast/expr.hpp"

#include <cassert>

namespace banjo {

namespace lang {

GenericsArgDeduction::GenericsArgDeduction(
    DataTypeManager &type_manager,
    const std::vector<GenericParam> &generic_params,
    bool has_sequence
)
  : type_manager(type_manager),
    generic_params(generic_params),
    has_sequence(has_sequence),
    generic_args(generic_params.size(), nullptr) {}

std::optional<std::vector<DataType *>> GenericsArgDeduction::deduce(
    const std::vector<ASTNode *> &param_nodes,
    const std::vector<DataType *> &arg_types
) {
    unsigned num_non_sequence_params = has_sequence ? param_nodes.size() - 1 : param_nodes.size();

    if (arg_types.size() < num_non_sequence_params) {
        return {};
    }

    for (unsigned i = 0; i < num_non_sequence_params; i++) {
        ASTNode *type_node = param_nodes[i]->get_child(PARAM_TYPE);
        deduce(type_node, arg_types[i]);
    }

    if (has_sequence) {
        Tuple sequence_tuple_type;

        for (unsigned i = param_nodes.size() - 1; i < arg_types.size(); i++) {
            sequence_tuple_type.types.push_back(arg_types[i]);
        }

        DataType *sequence_type = type_manager.new_data_type();
        sequence_type->set_to_tuple(sequence_tuple_type);
        generic_args.back() = sequence_type;
    }

    return {generic_args};
}

void GenericsArgDeduction::deduce(ASTNode *param_type, DataType *arg_type) {
    if (param_type->get_type() == AST_IDENTIFIER) {
        const std::string &identifier = param_type->as<Identifier>()->get_value();
        store_deduced_type(identifier, arg_type);
    } else if (param_type->get_type() == AST_STAR_EXPR) {
        deduce(param_type->get_child(), arg_type->get_base_data_type());
    }
}

void GenericsArgDeduction::store_deduced_type(const std::string &param_name, DataType *type) {
    auto it = std::find_if(generic_params.begin(), generic_params.end(), [&param_name](const GenericParam &param) {
        return param.name == param_name;
    });

    if (it != generic_params.end()) {
        unsigned index = std::distance(generic_params.begin(), it);
        generic_args[index] = type;
    }
}

} // namespace lang

} // namespace banjo
