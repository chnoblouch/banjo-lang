#include "assign_ir_builder.hpp"

#include "ast/ast_child_indices.hpp"
#include "ir_builder/expr_ir_builder.hpp"

namespace banjo {

namespace ir_builder {

void AssignIRBuilder::build() {
    lang::ASTNode *location_node = node->get_child(lang::ASSIGN_LOCATION);
    lang::ASTNode *value_node = node->get_child(lang::ASSIGN_VALUE);

    ir::Value dst = ExprIRBuilder(context, location_node).build_into_ptr().get_ptr();
    ExprIRBuilder(context, value_node).build_and_store(dst);
}

} // namespace ir_builder

} // namespace banjo
