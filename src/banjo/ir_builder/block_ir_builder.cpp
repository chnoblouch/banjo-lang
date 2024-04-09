#include "block_ir_builder.hpp"

#include "ast/ast_block.hpp"
#include "ast/ast_node.hpp"
#include "ir_builder/assign_ir_builder.hpp"
#include "ir_builder/compound_assign_ir_builder.hpp"
#include "ir_builder/for_ir_builder.hpp"
#include "ir_builder/func_call_ir_builder.hpp"
#include "ir_builder/if_chain_ir_builder.hpp"
#include "ir_builder/ir_builder_utils.hpp"
#include "ir_builder/return_ir_builder.hpp"
#include "ir_builder/switch_ir_builder.hpp"
#include "ir_builder/type_infer_var_decl_ir_builder.hpp"
#include "ir_builder/var_decl_ir_builder.hpp"
#include "ir_builder/while_ir_builder.hpp"
#include "symbol/symbol_table.hpp"

namespace ir_builder {

BlockIRBuilder::BlockIRBuilder(IRBuilderContext &context, lang::ASTNode *node)
  : IRBuilder(context, node),
    deinit_builder(context, node) {}

ir::BasicBlockIter BlockIRBuilder::create_exit() {
    int id = context.next_block_id();
    std::string exit_label = "block.exit." + std::to_string(id);
    exit = context.create_block(exit_label);
    return exit;
}

void BlockIRBuilder::build() {
    alloc_local_vars();
    deinit_builder.build();
    build_children();
    build_exit();
    deinit_builder.build_exit();
}

void BlockIRBuilder::alloc_local_vars() {
    if (local_vars_allocated) {
        return;
    }

    ir::Function *func = context.get_current_func();
    lang::SymbolTable *symbol_table = static_cast<lang::ASTBlock *>(node)->get_symbol_table();

    for (lang::LocalVariable *local_var : symbol_table->get_local_variables()) {
        ir::VirtualRegister reg = func->next_virtual_reg();
        local_var->set_virtual_reg_id(reg);
        ir::Type type = IRBuilderUtils::build_type(local_var->get_data_type(), context, false);
        context.append_alloca(reg, type);
    }

    local_vars_allocated = true;
}

void BlockIRBuilder::build_children() {
    for (lang::ASTNode *child : node->get_children()) {
        switch (child->get_type()) {
            case lang::AST_VAR: VarDeclIRBuilder(context, child).build(); break;
            case lang::AST_IMPLICIT_TYPE_VAR: TypeInferVarDeclIRBuilder(context, child).build(); break;
            case lang::AST_ASSIGNMENT: AssignIRBuilder(context, child).build(); break;
            case lang::AST_ADD_ASSIGN:
            case lang::AST_SUB_ASSIGN:
            case lang::AST_MUL_ASSIGN:
            case lang::AST_DIV_ASSIGN:
            case lang::AST_MOD_ASSIGN:
            case lang::AST_BIT_AND_ASSIGN:
            case lang::AST_BIT_OR_ASSIGN:
            case lang::AST_BIT_XOR_ASSIGN:
            case lang::AST_SHL_ASSIGN:
            case lang::AST_SHR_ASSIGN: CompoundAssignIRBuilder(context, child).build(); break;
            case lang::AST_IF_CHAIN: IfChainIRBuilder(context, child).build(); break;
            case lang::AST_SWITCH: SwitchIRBuilder(context, child).build(); break;
            case lang::AST_WHILE: WhileIRBuilder(context, child).build(); break;
            case lang::AST_FOR: ForIRBuilder(context, child).build(); break;
            case lang::AST_FUNCTION_CALL: FuncCallIRBuilder(context, child).build({}, false); break;
            case lang::AST_FUNCTION_RETURN: ReturnIRBuilder(context, child).build(); break;
            case lang::AST_BREAK: context.append_jmp(context.get_scope().loop_exit); break;
            case lang::AST_CONTINUE: context.append_jmp(context.get_scope().loop_entry); break;
            default: break;
        }
    }
}

void BlockIRBuilder::build_exit() {
    if (!exit) {
        create_exit();
    }

    context.append_jmp(exit);
    context.append_block(exit);
}

} // namespace ir_builder
