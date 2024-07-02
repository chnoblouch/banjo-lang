#include "generics_instantiator.hpp"

#include "ast/ast_block.hpp"
#include "ast/ast_child_indices.hpp"
#include "ast/ast_node.hpp"
#include "ast/ast_utils.hpp"
#include "ast/decl.hpp"
#include "ir_builder/name_mangling.hpp"
#include "sema/semantic_analyzer.hpp"
#include "sema/type_analyzer.hpp"

namespace banjo {

namespace lang {

GenericsInstantiator::GenericsInstantiator(SemanticAnalyzerContext &context) : context(context) {}

GenericFuncInstance *GenericsInstantiator::instantiate_func(GenericFunc *generic_func, ASTNode *arg_list) {
    std::optional<GenericInstanceArgs> args = get_arg_list(arg_list);
    if (!args) {
        return nullptr;
    }

    return instantiate_func(generic_func, *args);
}

GenericStructInstance *GenericsInstantiator::instantiate_struct(GenericStruct *generic_struct, ASTNode *arg_list) {
    std::optional<GenericInstanceArgs> args = get_arg_list(arg_list);
    if (!args) {
        return nullptr;
    }

    return instantiate_struct(generic_struct, *args);
}

GenericFuncInstance *GenericsInstantiator::instantiate_func(GenericFunc *generic_func, GenericInstanceArgs &args) {
    context.get_sema().run_type_stage(generic_func->get_node());

    GenericFuncInstance *instance = find_existing_instance(args, generic_func);

    if (!instance) {
        ASTNode *generic_func_node = generic_func->get_node();
        ASTNode *name_node = generic_func_node->get_child(GENERIC_FUNC_NAME);
        ASTNode *params_node = generic_func_node->get_child(GENERIC_FUNC_PARAMS);
        ASTNode *return_type_node = generic_func_node->get_child(GENERIC_FUNC_TYPE);
        ASTNode *block_node = generic_func_node->get_child(GENERIC_FUNC_BLOCK);
        SymbolTable *parent_symbol_table = ASTUtils::find_symbol_table(generic_func_node);

        ASTFunc *func_node = new ASTFunc(AST_FUNCTION_DEFINITION);
        func_node->set_range(generic_func_node->get_range());
        func_node->set_parent(generic_func_node->get_parent());
        func_node->append_child(new ASTNode(AST_QUALIFIER_LIST));
        func_node->append_child(name_node->clone());
        func_node->append_child(params_node->clone());
        func_node->append_child(return_type_node->clone());

        ASTBlock *block = block_node->as<ASTBlock>();
        ASTBlock *block_clone = block->clone_block(parent_symbol_table);
        func_node->append_child(block_clone);

        instance = new GenericFuncInstance(generic_func, args);
        generic_func->add_instance(instance);

        ASTContext &instance_context = context.push_ast_context();
        instance_context.set_generic_types(instance);
        instance_context.cur_module = generic_func->get_module();
        instance_context.enclosing_symbol = {};

        context.get_sema().run_name_stage(func_node, false);
        context.get_sema().run_type_stage(func_node);
        context.get_new_nodes().push_back({func_node, instance_context});

        context.pop_ast_context();

        Function *func = func_node->as<lang::ASTFunc>()->get_symbol();
        instance->entity = func;
        func->set_generic_instance(instance);

        std::string id = std::to_string((int)generic_func->get_instances().size());
        func->set_link_name(ir_builder::NameMangling::mangle_func_name(func) + "$" + id);
    }

    return instance;
}

GenericStructInstance *GenericsInstantiator::instantiate_struct(
    GenericStruct *generic_struct,
    GenericInstanceArgs &args
) {
    context.get_sema().run_type_stage(generic_struct->get_node());

    GenericStructInstance *instance = find_existing_instance(args, generic_struct);

    if (!instance) {
        ASTNode *generic_struct_node = generic_struct->get_node();
        ASTNode *name_node = generic_struct_node->get_child(GENERIC_STRUCT_NAME);
        ASTNode *block_node = generic_struct_node->get_child(GENERIC_STRUCT_BLOCK);
        SymbolTable *parent_symbol_table = ASTUtils::find_symbol_table(generic_struct_node);

        ASTStruct *struct_node = new ASTStruct();
        struct_node->set_range(generic_struct_node->get_range());
        struct_node->set_parent(generic_struct_node->get_parent());
        struct_node->append_child(name_node->clone());
        struct_node->append_child(new ASTNode(AST_IMPL_LIST));

        ASTBlock *block = block_node->as<ASTBlock>();
        ASTBlock *block_clone = block->clone_block(parent_symbol_table);
        struct_node->append_child(block_clone);

        instance = new GenericStructInstance(generic_struct, args);
        instance->entity = struct_node->get_symbol();
        generic_struct->add_instance(instance);

        ASTContext &instance_context = context.push_ast_context();
        instance_context.set_generic_types(instance);
        instance_context.cur_module = generic_struct->get_module();

        context.get_sema().run_name_stage(struct_node, false);
        context.get_sema().run_type_stage(struct_node);
        context.get_new_nodes().push_back({struct_node, instance_context});

        std::string name = generic_struct->get_name();
        name += std::to_string(generic_struct->get_instances().size());

        /*
        for(unsigned int i = 0; i < args.size(); i++) {
            name += ReportUtils::type_to_string(args[i]);

            if(i != args.size() - 1) {
                name += ", ";
            }
        }
        name += ">";
        */

        struct_node->get_symbol()->set_name(name);

        context.pop_ast_context();
    }

    return instance;
}

std::optional<GenericInstanceArgs> GenericsInstantiator::get_arg_list(ASTNode *node) {
    GenericInstanceArgs args;

    for (ASTNode *child : node->get_children()) {
        TypeAnalyzer::Result type_result = TypeAnalyzer(context).analyze(child);
        if (type_result.result != SemaResult::OK) {
            return {};
        }
        DataType *type = type_result.type;

        args.push_back(type);
    }

    return args;
}

} // namespace lang

} // namespace banjo
