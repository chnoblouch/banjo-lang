#ifndef IR_BUILDER_ROOT_IR_BUILDER_H
#define IR_BUILDER_ROOT_IR_BUILDER_H

#include "banjo/ast/ast_module.hpp"
#include "banjo/ast/ast_node.hpp"
#include "banjo/ast/module_list.hpp"
#include "banjo/ir/module.hpp"
#include "banjo/ir_builder/ir_builder_context.hpp"
#include "banjo/target/target.hpp"

namespace banjo {

namespace ir_builder {

class RootIRBuilder {

private:
    IRBuilderContext context;
    lang::ModuleList &module_list;

public:
    RootIRBuilder(lang::ModuleList &module_list, target::Target *target) : context(target), module_list(module_list) {}
    ir::Module build();

private:
    void insert_runtime_ir();

    void create_ir_types(ir::Module &ir_mod, lang::ASTModule *module_node);
    void create_ir_struct(ir::Module &ir_mod, lang::Structure *lang_struct);
    void create_ir_union(ir::Module &ir_mod, lang::Union *lang_union);
    void create_ir_structs(ir::Module &ir_mod, lang::GenericStruct *lang_generic_struct);
    void create_ir_funcs(lang::ASTNode *module_node);

    void create_ir_struct_members(lang::ASTNode *module_node);
    void create_ir_union_case_members(lang::ASTNode *module_node);
    void create_ir_union_members(lang::ASTNode *module_node);
    
    void build_ir_vtables(ir::Module &ir_mod, lang::ASTNode *module_node);

    void build_ir_entities(ir::Module &ir_mod, lang::ASTModule *ast_module);
    void build_func(lang::ASTNode *node);
    void build_native_func(ir::Module &ir_mod, lang::ASTNode *node);
    void build_generic_func(lang::ASTNode *node);
    void build_global(ir::Module &ir_mod, lang::ASTNode *node, lang::SymbolTable *symbol_table);
    void build_native_global(ir::Module &ir_mod, lang::ASTNode *node, lang::SymbolTable *symbol_table);
    void build_struct(lang::ASTNode *node);
    void build_generic_struct(lang::ASTNode *node);
    void build_union(lang::ASTNode *node);
};

} // namespace ir_builder

} // namespace banjo

#endif
