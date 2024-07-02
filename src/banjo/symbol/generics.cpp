#include "generics.hpp"

#include "ast/ast_child_indices.hpp"
#include "ast/ast_node.hpp"

namespace banjo {

namespace lang {

bool GenericsUtils::has_sequence(GenericFunc *func) {
    ASTNode *param_list = func->get_node()->get_child(GENERIC_FUNC_PARAMS);

    if (param_list->get_children().empty()) {
        return false;
    }

    ASTNode *param_node = param_list->get_children().back();
    if (param_node->get_children().size() == 1) {
        return false;
    }

    ASTNode *param_type_node = param_node->get_child(PARAM_TYPE);
    if (param_type_node->get_type() != AST_IDENTIFIER) {
        return false;
    }

    const std::string &sequence_param_name = param_type_node->get_value();

    for (const GenericParam &generic_param : func->get_generic_params()) {
        if (generic_param.name == sequence_param_name && generic_param.is_param_sequence) {
            return true;
        }
    }

    return false;
}

} // namespace lang

} // namespace banjo
