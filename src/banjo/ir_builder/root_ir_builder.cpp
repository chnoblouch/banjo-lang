#include "root_ir_builder.hpp"

#include "ast/ast_block.hpp"
#include "ast/ast_child_indices.hpp"
#include "ast/ast_module.hpp"
#include "ast/ast_node.hpp"
#include "ast/ast_utils.hpp"
#include "ast/decl.hpp"
#include "ir/primitive.hpp"
#include "ir/structure.hpp"
#include "ir_builder/expr_ir_builder.hpp"
#include "ir_builder/func_def_ir_builder.hpp"
#include "ir_builder/ir_builder_utils.hpp"
#include "ir_builder/name_mangling.hpp"
#include "ir_builder/runtime_ir.hpp"
#include "ir_builder/struct_def_ir_builder.hpp"
#include "passes/precomputing.hpp"
#include "symbol/data_type.hpp"
#include "symbol/generics.hpp"
#include "symbol/symbol_ref.hpp"
#include "symbol/symbol_table.hpp"
#include "symbol/union.hpp"

namespace ir_builder {

ir::Module RootIRBuilder::build() {
    ir::Module ir_mod;
    context.set_current_mod(&ir_mod);

    RuntimeIR::insert(context);

    for (lang::ASTModule *module_node : module_list) {
        create_ir_types(ir_mod, module_node);
    }

    for (lang::ASTModule *module_node : module_list) {
        lang::SymbolTable *symbol_table = module_node->get_block()->get_symbol_table();

        for (const auto &[name, symbol] : symbol_table->get_symbols()) {
            if (symbol.get_kind() == lang::SymbolKind::UNION) {
                create_ir_union(ir_mod, symbol.get_union());
            }
        }
    }

    for (lang::ASTModule *module_node : module_list) {
        create_ir_struct_members(module_node);
        create_ir_union_case_members(module_node);
    }

    for (lang::ASTModule *module_node : module_list) {
        create_ir_union_members(module_node);
    }

    for (lang::ASTModule *module_node : module_list) {
        create_ir_funcs(module_node);
    }

    for (lang::ASTModule *module_node : module_list) {
        context.set_current_mod_path(module_node->get_path());
        build_ir_entities(ir_mod, module_node);
    }

    for (ir::Function *func : ir_mod.get_functions()) {
        passes::Precomputing::precompute_instrs(*func);
    }

    return ir_mod;
}

void RootIRBuilder::create_ir_types(ir::Module &ir_mod, lang::ASTModule *module_node) {
    lang::SymbolTable *symbol_table = module_node->get_block()->get_symbol_table();

    lang::ASTUtils::iterate_structs(symbol_table, [this, &ir_mod](lang::Structure *lang_struct) {
        create_ir_struct(ir_mod, lang_struct);
    });
}

void RootIRBuilder::create_ir_struct(ir::Module &ir_mod, lang::Structure *lang_struct) {
    ir::Structure *ir_struct = new ir::Structure(NameMangling::mangle_struct_name(lang_struct));
    lang_struct->set_ir_struct(ir_struct);
    ir_mod.add(ir_struct);
}

void RootIRBuilder::create_ir_union(ir::Module &ir_mod, lang::Union *lang_union) {
    for (lang::UnionCase *lang_case : lang_union->get_cases()) {
        ir::Structure *ir_case = new ir::Structure(NameMangling::mangle_union_case_name(lang_case));
        lang_case->set_ir_struct(ir_case);
        ir_mod.add(ir_case);
    }

    ir::Structure *ir_union = new ir::Structure(NameMangling::mangle_union_name(lang_union));
    lang_union->set_ir_struct(ir_union);
    ir_mod.add(ir_union);
}

void RootIRBuilder::create_ir_structs(ir::Module &ir_mod, lang::GenericStruct *lang_generic_struct) {
    for (lang::GenericStructInstance *instance : lang_generic_struct->get_instances()) {
        create_ir_struct(ir_mod, instance->entity);
    }
}

void RootIRBuilder::create_ir_funcs(lang::ASTNode *module_node) {
    lang::ASTNode *block_node = module_node->get_child_of_type(lang::AST_BLOCK);
    lang::SymbolTable *symbol_table = block_node->as<lang::ASTBlock>()->get_symbol_table();

    lang::ASTUtils::iterate_funcs(symbol_table, [this](lang::Function *lang_func) {
        if (!lang_func->is_native()) {
            lang_func->set_ir_func(new ir::Function());
        }

        ir::Type type = IRBuilderUtils::build_type(lang_func->get_type().return_type);
        if (context.get_target()->get_data_layout().is_return_by_ref(type)) {
            lang_func->set_return_by_ref(true);
        }
    });
}

void RootIRBuilder::create_ir_struct_members(lang::ASTNode *module_node) {
    lang::SymbolTable *symbol_table = lang::ASTUtils::get_module_symbol_table(module_node);

    lang::ASTUtils::iterate_structs(symbol_table, [](lang::Structure *lang_struct) {
        ir::Structure *ir_struct = lang_struct->get_ir_struct();

        for (lang::StructField *lang_field : lang_struct->get_fields()) {
            ir::Type ir_type = IRBuilderUtils::build_type(lang_field->get_type());
            ir_struct->add(ir::StructureMember{lang_field->get_name(), ir_type});
        }
    });
}

void RootIRBuilder::create_ir_union_case_members(lang::ASTNode *module_node) {
    lang::SymbolTable *symbol_table = lang::ASTUtils::get_module_symbol_table(module_node);

    lang::ASTUtils::iterate_unions(symbol_table, [](lang::Union *lang_union) {
        for (lang::UnionCase *lang_case : lang_union->get_cases()) {
            ir::Structure *ir_case = lang_case->get_ir_struct();

            for (lang::UnionCaseField *lang_field : lang_case->get_fields()) {
                ir::Type ir_type = IRBuilderUtils::build_type(lang_field->get_type());
                ir_case->add(ir::StructureMember{lang_field->get_name(), ir_type});
            }
        }
    });
}

void RootIRBuilder::create_ir_union_members(lang::ASTNode *module_node) {
    lang::SymbolTable *symbol_table = lang::ASTUtils::get_module_symbol_table(module_node);

    lang::ASTUtils::iterate_unions(symbol_table, [this](lang::Union *lang_union) {
        ir::Structure *ir_union = lang_union->get_ir_struct();

        unsigned largest_size = 0;

        for (lang::UnionCase *lang_case : lang_union->get_cases()) {
            ir::Structure *ir_case = lang_case->get_ir_struct();

            // FIXME: the size calculation breaks down if not all the types in the union cases are defined.
            unsigned size = context.get_target()->get_data_layout().get_size(ir_case);
            largest_size = std::max(largest_size, size);
        }

        ir_union->add(ir::StructureMember{"tag", ir::Primitive::I32});
        ir_union->add(ir::StructureMember{"data", ir::Type(ir::Primitive::I8, 0, largest_size)});
    });
}

void RootIRBuilder::build_ir_entities(ir::Module &ir_mod, lang::ASTModule *ast_module) {
    lang::ASTBlock *block = ast_module->get_block();
    lang::SymbolTable *symbol_table = block->get_symbol_table();

    for (lang::ASTNode *child : block->get_children()) {
        switch (child->get_type()) {
            case lang::AST_FUNCTION_DEFINITION: build_func(child); break;
            case lang::AST_NATIVE_FUNCTION_DECLARATION: build_native_func(ir_mod, child); break;
            case lang::AST_GENERIC_FUNCTION_DEFINITION: build_generic_func(child); break;
            case lang::AST_VAR: build_global(ir_mod, child, symbol_table); break;
            case lang::AST_NATIVE_VAR: build_native_global(ir_mod, child, symbol_table); break;
            case lang::AST_STRUCT_DEFINITION: build_struct(child); break;
            case lang::AST_GENERIC_STRUCT_DEFINITION: build_generic_struct(child); break;
            case lang::AST_UNION: build_union(child); break;
            default: break;
        }
    }
}

void RootIRBuilder::build_func(lang::ASTNode *node) {
    FuncDefIRBuilder(context, node).build();
}

void RootIRBuilder::build_native_func(ir::Module &ir_mod, lang::ASTNode *node) {
    lang::Function *func = node->as<lang::ASTFunc>()->get_symbol();
    if (!func->is_used()) {
        return;
    }

    std::string name = IRBuilderUtils::get_func_link_name(func);
    std::vector<ir::Type> param_list = IRBuilderUtils::build_params(func->get_type().param_types, context);
    ir::Type return_type = IRBuilderUtils::build_type(func->get_type().return_type);

    if (context.get_target()->get_data_layout().is_return_by_ref(return_type)) {
        param_list.insert(param_list.begin(), return_type.ref());
        return_type = ir::Type(ir::Primitive::VOID);
        func->set_return_by_ref(true);
    }

    // TODO: calling conv
    ir_mod.add(ir::FunctionDecl(name, param_list, return_type, ir::CallingConv::X86_64_MS_ABI));
}

void RootIRBuilder::build_generic_func(lang::ASTNode *node) {
    lang::GenericFunc *generic_func = node->as<lang::ASTGenericFunc>()->get_symbol();

    for (lang::GenericFuncInstance *instance : generic_func->get_instances()) {
        FuncDefIRBuilder(context, instance->entity->get_node()).build();
    }
}

void RootIRBuilder::build_global(ir::Module &ir_mod, lang::ASTNode *node, lang::SymbolTable *symbol_table) {
    lang::ASTNode *name_node = node->get_child(lang::VAR_NAME);
    lang::ASTNode *value_node = node->get_child(lang::VAR_VALUE);

    std::string name = name_node->get_value();
    std::optional<lang::SymbolRef> symbol = symbol_table->get_symbol(name);
    lang::GlobalVariable *var = symbol->get_global();

    std::string link_name = IRBuilderUtils::get_global_var_link_name(var);
    ir::Type type = IRBuilderUtils::build_type(var->get_data_type());
    ir::Value value = ExprIRBuilder(context, value_node).build_into_value_if_possible().value_or_ptr;

    ir::Global global(link_name, type, value);
    global.set_external(var->is_exposed());

    ir_mod.add(global);
}

void RootIRBuilder::build_native_global(ir::Module &ir_mod, lang::ASTNode *node, lang::SymbolTable *symbol_table) {
    std::string name = node->get_child(lang::VAR_NAME)->get_value();
    std::optional<lang::SymbolRef> symbol = symbol_table->get_symbol(name);
    lang::GlobalVariable *var = symbol->get_global();

    ir_mod.add(
        ir::GlobalDecl(IRBuilderUtils::get_global_var_link_name(var), IRBuilderUtils::build_type(var->get_data_type()))
    );
}

void RootIRBuilder::build_struct(lang::ASTNode *node) {
    StructDefIRBuilder(context, node).build();
}

void RootIRBuilder::build_generic_struct(lang::ASTNode *node) {
    lang::GenericStruct *generic_struct = node->as<lang::ASTGenericStruct>()->get_symbol();

    for (lang::GenericStructInstance *instance : generic_struct->get_instances()) {
        build_struct(instance->entity->get_node());
    }
}

void RootIRBuilder::build_union(lang::ASTNode *node) {
    lang::ASTNode *block_node = node->get_child(lang::UNION_BLOCK);

    for (lang::ASTNode *child : block_node->get_children()) {
        if (child->get_type() == lang::AST_FUNCTION_DEFINITION) {
            FuncDefIRBuilder(context, child).build();
        }
    }
}

} // namespace ir_builder
