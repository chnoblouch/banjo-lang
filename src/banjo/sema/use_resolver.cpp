#include "use_resolver.hpp"

#include "banjo/ast/ast_child_indices.hpp"
#include "banjo/ast/ast_module.hpp"
#include "banjo/ast/ast_node.hpp"
#include "banjo/ast/expr.hpp"
#include "banjo/symbol/symbol_table.hpp"
#include "banjo/symbol/use.hpp"

namespace banjo {

namespace lang {

UseResolver::UseResolver(SemanticAnalyzerContext &context) : context(context) {}

void UseResolver::run(ASTNode *node) {
    if (node->get_sema_stage() != SemaStage::NAME) {
        return;
    }
    node->set_sema_stage(SemaStage::BODY);

    use_node = node;

    ASTNode *path_node = node->get_child(0);
    SymbolTable *symbol_table = context.get_ast_context().get_cur_symbol_table();
    resolve_names(path_node, {.first = true, .final_comp = true}, symbol_table);
}

void UseResolver::resolve_names(ASTNode *node, PathContext ctx, SymbolTable *&symbol_table) {
    switch (node->get_type()) {
        case AST_IDENTIFIER: resolve_ident_names(node, ctx, symbol_table); break;
        case AST_DOT_OPERATOR: resolve_dot_names(node, ctx, symbol_table); break;
        case AST_USE_TREE_LIST: resolve_tree_list(node, symbol_table); break;
        case AST_USE_REBINDING: resolve_rebinding(node, ctx, symbol_table); break;
        default: break;
    }
}

void UseResolver::resolve_ident_names(ASTNode *node, PathContext ctx, SymbolTable *&symbol_table) {
    const std::string &name = node->get_value();

    std::optional<SymbolRef> symbol;
    if (ctx.first) {
        ASTModule *module_ = context.get_module_manager().get_module_list().get_by_path({name});
        symbol = module_ ? std::optional<SymbolRef>{SymbolRef(module_)} : std::optional<SymbolRef>{};
    } else {
        symbol = symbol_table->get_symbol(name);
    }

    if (!symbol) {
        if (context.is_complete(symbol_table)) {
            context.register_error(node->get_range()).set_message(ReportText::ID::ERR_USE_NO_SYMBOL);
        } else {
            context.register_error(node->get_range()).set_message("incomplete");
        }

        return;
    }

    if (symbol->get_kind() == SymbolKind::MODULE) {
        symbol_table = symbol->get_module()->get_block()->get_symbol_table();

        if (context.is_track_dependencies()) {
            symbol->get_module()->add_dependent(context.get_ast_context().cur_module);
        }
    }

    symbol = symbol->as_link();
    context.add_symbol_use(*symbol, node->as<Identifier>());

    if (ctx.final_comp) {
        node->as<Identifier>()->get_symbol()->get_use()->set_target(*symbol);
    } else {
        node->as<Identifier>()->set_symbol(*symbol);
    }
}

void UseResolver::resolve_dot_names(ASTNode *node, PathContext ctx, SymbolTable *&symbol_table) {
    resolve_names(node->get_child(0), {.first = ctx.first, .final_comp = false}, symbol_table);
    resolve_names(node->get_child(1), {.first = false, .final_comp = ctx.final_comp}, symbol_table);
}

void UseResolver::resolve_tree_list(ASTNode *node, SymbolTable *&symbol_table) {
    for (ASTNode *child : node->get_children()) {
        SymbolTable *symbol_table_ref_copy = symbol_table;
        resolve_names(child, {.first = false, .final_comp = true}, symbol_table_ref_copy);
    }
}

void UseResolver::resolve_rebinding(ASTNode *node, PathContext ctx, SymbolTable *&symbol_table) {
    ASTNode *target_node = node->get_child(USE_REBINDING_TARGET);
    ASTNode *local_node = node->get_child(USE_REBINDING_LOCAL);

    const std::string &target_name = target_node->get_value();

    std::optional<SymbolRef> symbol;
    if (ctx.first) {
        ASTModule *module_ = context.get_module_manager().get_module_list().get_by_path({target_name});
        symbol = module_ ? std::optional<SymbolRef>{SymbolRef(module_)} : std::optional<SymbolRef>{};
    } else {
        symbol = symbol_table->get_symbol(target_name);
    }

    if (!symbol) {
        if (context.is_complete(symbol_table)) {
            context.register_error(node->get_range()).set_message(ReportText::ID::ERR_USE_NO_SYMBOL);
        } else {
            context.register_error(node->get_range()).set_message("incomplete");
        }

        return;
    }

    if (symbol->get_kind() == SymbolKind::MODULE) {
        symbol_table = symbol->get_module()->get_block()->get_symbol_table();

        if (context.is_track_dependencies()) {
            symbol->get_module()->add_dependent(context.get_ast_context().cur_module);
        }
    }

    symbol = symbol->as_link();
    context.add_symbol_use(*symbol, local_node->as<Identifier>());

    if (ctx.final_comp) {
        local_node->as<Identifier>()->get_symbol()->get_use()->set_target(*symbol);
    } else {
        target_node->as<Identifier>()->set_symbol(*symbol);
        local_node->as<Identifier>()->set_symbol(*symbol);
    }
}

} // namespace lang

} // namespace banjo
