#include "var_decl_ir_builder.hpp"

#include "banjo/ast/ast_child_indices.hpp"
#include "banjo/ast/decl.hpp"
#include "banjo/ir_builder/expr_ir_builder.hpp"

namespace banjo {

namespace ir_builder {

void VarDeclIRBuilder::build() {
    // Return if there's no value as there's nothing to build.
    if (node->get_children().size() < 3) {
        return;
    }

    lang::ASTNode *val_node = node->get_child(lang::VAR_VALUE);

    lang::LocalVariable *var = static_cast<lang::LocalVariable *>(node->as<lang::ASTVar>()->get_symbol());
    ExprIRBuilder(context, val_node).build_and_store(var->get_virtual_reg());
}

} // namespace ir_builder

} // namespace banjo
