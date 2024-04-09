#include "var_decl_ir_builder.hpp"

#include "ast/ast_child_indices.hpp"
#include "ast/decl.hpp"
#include "ir_builder/expr_ir_builder.hpp"

namespace ir_builder {

void VarDeclIRBuilder::build() {
    // Return if there's no value as there's nothing to build.
    if (node->get_children().size() < 3) {
        return;
    }

    lang::ASTNode *val_node = node->get_child(lang::VAR_VALUE);

    lang::LocalVariable *var = static_cast<lang::LocalVariable *>(node->as<lang::ASTVar>()->get_symbol());
    ir::VirtualRegister dest_reg = ir::VirtualRegister(var->get_virtual_reg_id());
    ExprIRBuilder(context, val_node).build_and_store(dest_reg);
}

} // namespace ir_builder
