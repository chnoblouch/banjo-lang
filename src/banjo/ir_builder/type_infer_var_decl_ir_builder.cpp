#include "type_infer_var_decl_ir_builder.hpp"

#include "banjo/ast/ast_child_indices.hpp"
#include "banjo/ast/decl.hpp"
#include "banjo/ir_builder/expr_ir_builder.hpp"
#include "banjo/symbol/local_variable.hpp"

namespace banjo {

namespace ir_builder {

void TypeInferVarDeclIRBuilder::build() {
    lang::ASTNode *val_node = node->get_child(lang::TYPE_INFERRED_VAR_VALUE);

    lang::LocalVariable *var = static_cast<lang::LocalVariable *>(node->as<lang::ASTVar>()->get_symbol());
    ExprIRBuilder(context, val_node).build_and_store(var->get_virtual_reg());
}

} // namespace ir_builder

} // namespace banjo
