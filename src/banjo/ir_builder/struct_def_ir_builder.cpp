#include "struct_def_ir_builder.hpp"

#include "ast/ast_child_indices.hpp"
#include "ast/decl.hpp"
#include "ir/structure.hpp"
#include "ir_builder/func_def_ir_builder.hpp"

namespace ir_builder {

void StructDefIRBuilder::build() {
    lang::ASTNode *block_node = node->get_child(lang::STRUCT_BLOCK);

    lang::Structure *lang_struct = node->as<lang::ASTStruct>()->get_symbol();
    ir::Structure *ir_struct = lang_struct->get_ir_struct();

    context.set_current_lang_struct(lang_struct);
    context.set_current_struct(ir_struct);

    for (lang::ASTNode *child : block_node->get_children()) {
        if (child->get_type() == lang::AST_FUNCTION_DEFINITION) {
            FuncDefIRBuilder(context, child).build();
        }
    }

    context.set_current_lang_struct(nullptr);
    context.set_current_struct(nullptr);
}

} // namespace ir_builder
