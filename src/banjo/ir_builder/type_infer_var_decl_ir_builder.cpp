#include "type_infer_var_decl_ir_builder.hpp"

#include "ast/ast_child_indices.hpp"
#include "ast/decl.hpp"
#include "ir_builder/expr_ir_builder.hpp"
#include "symbol/local_variable.hpp"

namespace ir_builder {

void TypeInferVarDeclIRBuilder::build() {
    lang::ASTNode *val_node = node->get_child(lang::TYPE_INFERRED_VAR_VALUE);

    lang::LocalVariable *var = static_cast<lang::LocalVariable *>(node->as<lang::ASTVar>()->get_symbol());
    ExprIRBuilder(context, val_node).build_and_store(var->get_virtual_reg());
}

} // namespace ir_builder
