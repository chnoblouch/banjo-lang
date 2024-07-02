#include "ast_utils.hpp"

#include "banjo/ast/ast_block.hpp"
#include "banjo/ast/ast_child_indices.hpp"
#include "banjo/ast/ast_module.hpp"
#include "banjo/ast/ast_node.hpp"
#include "banjo/ast/decl.hpp"
#include "banjo/symbol/function.hpp"
#include "banjo/symbol/symbol_ref.hpp"
#include "banjo/symbol/union.hpp"

namespace banjo {

namespace lang {

ModulePath ASTUtils::get_path_from_node(ASTNode *node) {
    if (node->get_type() == AST_IDENTIFIER) {
        return ModulePath({node->get_value()});
    } else if (node->get_type() == AST_DOT_OPERATOR) {
        ModulePath path;
        append_path_from_node(node->get_child(0), path);
        append_path_from_node(node->get_child(1), path);
        return path;
    } else {
        return ModulePath();
    }
}

void ASTUtils::append_path_from_node(ASTNode *node, ModulePath &dest) {
    if (node->get_type() == AST_IDENTIFIER) {
        dest.append(node->get_value());
    } else if (node->get_type() == AST_DOT_OPERATOR) {
        append_path_from_node(node->get_child(0), dest);
        append_path_from_node(node->get_child(1), dest);
    }
}

SymbolTable *ASTUtils::find_symbol_table(ASTNode *node) {
    if (node->get_type() == AST_BLOCK) {
        return node->as<ASTBlock>()->get_symbol_table();
    }

    return node->get_parent() ? find_symbol_table(node->get_parent()) : nullptr;
}

ASTModule *ASTUtils::find_module(ASTNode *node) {
    ASTNode *parent = node->get_parent();

    while (parent) {
        if (parent->get_type() == AST_MODULE) {
            return parent->as<ASTModule>();
        } else {
            parent = parent->get_parent();
        }
    }

    return nullptr;
}

SymbolTable *ASTUtils::get_module_symbol_table(ASTNode *module_node) {
    ASTNode *block_node = module_node->get_child_of_type(AST_BLOCK);
    return block_node->as<ASTBlock>()->get_symbol_table();
}

Symbol *ASTUtils::get_symbol(ASTNode *node) {
    switch (node->get_type()) {
        case AST_FUNCTION_DEFINITION: return node->as<ASTFunc>()->get_symbol();
        case AST_NATIVE_FUNCTION_DECLARATION: return node->as<ASTFunc>()->get_symbol();
        case AST_CONSTANT: return node->as<ASTConst>()->get_symbol();
        case AST_VAR: return node->as<ASTVar>()->get_symbol();
        case AST_IMPLICIT_TYPE_VAR: return node->as<ASTVar>()->get_symbol();
        case AST_NATIVE_VAR: return node->as<ASTVar>()->get_symbol();
        case AST_STRUCT_DEFINITION: return node->as<ASTStruct>()->get_symbol();
        case AST_ENUM_DEFINITION: return node->as<ASTEnum>()->get_symbol();
        case AST_UNION: return node->as<ASTUnion>()->get_symbol();
        case AST_PROTO: return node->as<ASTProto>()->get_symbol();
        case AST_TYPE_ALIAS: return node->as<ASTTypeAlias>()->get_symbol();
        case AST_GENERIC_FUNCTION_DEFINITION: return node->as<ASTGenericFunc>()->get_symbol();
        case AST_GENERIC_STRUCT_DEFINITION: return node->as<ASTGenericStruct>()->get_symbol();
        default: return nullptr;
    }
}

bool ASTUtils::is_func_method(ASTNode *node) {
    ASTNode *params_list_node = node->get_child(FUNC_PARAMS);
    if (params_list_node->get_children().empty()) {
        return false;
    }

    ASTNode *first_param_node = params_list_node->get_child(0);
    if (first_param_node->get_children().size() != 1) {
        return false;
    }

    return first_param_node->get_child()->get_type() == AST_SELF;
}

GenericStruct *ASTUtils::resolve_generic_struct(const ModulePath &path, SymbolTable *symbol_table) {
    SymbolTable *current_symbol_table = symbol_table;

    for (std::string element : path.get_path()) {
        std::optional<SymbolRef> symbol = current_symbol_table->get_symbol(element);
        if (!symbol) {
            return nullptr;
        }

        if (symbol->get_kind() == SymbolKind::GENERIC_STRUCT) {
            return symbol->get_generic_struct();
        } else if (symbol->get_kind() == SymbolKind::MODULE) {
            current_symbol_table = get_module_symbol_table(symbol->get_module());
        } else {
            // TODO: error message
            return nullptr;
        }
    }

    return nullptr;
}

void ASTUtils::iterate_funcs(lang::SymbolTable *symbol_table, std::function<void(lang::Function *)> callback) {
    for (lang::Function *func : symbol_table->get_functions()) {
        callback(func);
    }

    for (lang::GenericFunc *generic_func : symbol_table->get_generic_funcs()) {
        for (lang::GenericFuncInstance *instance : generic_func->get_instances()) {
            callback(instance->entity);
        }
    }

    iterate_structs(symbol_table, [&callback](lang::Structure *struct_) {
        for (lang::Function *func : struct_->get_method_table().get_functions()) {
            callback(func);
        }

        for (lang::Function *func : struct_->get_symbol_table()->get_functions()) {
            callback(func);
        }
    });

    iterate_unions(symbol_table, [&callback](lang::Union *union_) {
        for (lang::Function *func : union_->get_method_table().get_functions()) {
            callback(func);
        }

        for (lang::Function *func : union_->get_symbol_table()->get_functions()) {
            callback(func);
        }
    });
}

void ASTUtils::iterate_structs(lang::SymbolTable *symbol_table, std::function<void(lang::Structure *)> callback) {
    for (lang::Structure *struct_ : symbol_table->get_structures()) {
        callback(struct_);
    }

    for (lang::GenericStruct *generic_struct : symbol_table->get_generic_structs()) {
        for (lang::GenericStructInstance *instance : generic_struct->get_instances()) {
            callback(instance->entity);
        }
    }
}

void ASTUtils::iterate_unions(lang::SymbolTable *symbol_table, std::function<void(lang::Union *)> callback) {
    for (const auto &[name, symbol] : symbol_table->get_symbols()) {
        if (symbol.get_kind() == SymbolKind::UNION) {
            callback(symbol.get_union());
        }
    }
}

void ASTUtils::iterate_protos(lang::SymbolTable *symbol_table, std::function<void(lang::Protocol *)> callback) {
    for (const auto &[name, symbol] : symbol_table->get_symbols()) {
        if (symbol.get_kind() == SymbolKind::PROTO) {
            callback(symbol.get_proto());
        }
    }
}

} // namespace lang

} // namespace banjo
