#include "assign_ir_builder.hpp"

#include "ast/ast_child_indices.hpp"
#include "ir_builder/expr_ir_builder.hpp"

namespace ir_builder {

void AssignIRBuilder::build() {
    lang::ASTNode *location_node = node->get_child(lang::ASSIGN_LOCATION);
    lang::ASTNode *value_node = node->get_child(lang::ASSIGN_VALUE);

    StoredValue dst = ExprIRBuilder(context, location_node).build(StorageReqs::FORCE_REFERENCE);
    ExprIRBuilder(context, value_node).build_and_store(dst.value_or_ptr);
}

} // namespace ir_builder
