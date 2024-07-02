#include "use_collector.hpp"

#include "banjo/ast/ast_child_indices.hpp"
#include "banjo/ast/ast_node.hpp"
#include "banjo/ast/expr.hpp"
#include "banjo/symbol/symbol_table.hpp"
#include "banjo/symbol/use.hpp"

namespace banjo {

namespace lang {

UseCollector::UseCollector(SemanticAnalyzerContext &context) : context(context) {}

void UseCollector::run(ASTNode *node) {
    use_node = node;

    ASTNode *path_node = node->get_child(0);
    SymbolTable *symbol_table = context.get_ast_context().get_cur_symbol_table();
    analyze_names(path_node, true, symbol_table);
}

void UseCollector::analyze_names(ASTNode *node, bool final_comp, SymbolTable *symbol_table) {
    switch (node->get_type()) {
        case AST_IDENTIFIER: analyze_ident_names(node, final_comp, symbol_table); break;
        case AST_DOT_OPERATOR: analyze_dot_names(node, final_comp, symbol_table); break;
        case AST_USE_TREE_LIST: analyze_tree_list(node, symbol_table); break;
        case AST_USE_REBINDING: analyze_rebinding(node, final_comp, symbol_table); break;
        default: break;
    }
}

void UseCollector::analyze_ident_names(ASTNode *node, bool final_comp, SymbolTable *symbol_table) {
    const std::string &name = node->get_value();

    if (final_comp) {
        SymbolRef symbol = new Use(use_node, name);
        symbol_table->add_symbol(name, symbol);
        node->as<Identifier>()->set_symbol(symbol.as_link());
    }
}

void UseCollector::analyze_dot_names(ASTNode *node, bool final_comp, SymbolTable *symbol_table) {
    analyze_names(node->get_child(0), false, symbol_table);
    analyze_names(node->get_child(1), final_comp, symbol_table);
}

void UseCollector::analyze_tree_list(ASTNode *node, SymbolTable *symbol_table) {
    for (ASTNode *child : node->get_children()) {
        analyze_names(child, true, symbol_table);
    }
}

void UseCollector::analyze_rebinding(ASTNode *node, bool final_comp, SymbolTable *symbol_table) {
    ASTNode *target_node = node->get_child(USE_REBINDING_TARGET);
    ASTNode *local_node = node->get_child(USE_REBINDING_LOCAL);

    const std::string &local_name = local_node->get_value();

    if (final_comp) {
        SymbolRef symbol = new Use(use_node, local_name);
        symbol_table->add_symbol(local_name, symbol);

        target_node->as<Identifier>()->set_symbol(symbol.as_link());
        local_node->as<Identifier>()->set_symbol(symbol.as_link());
    }
}

} // namespace lang

} // namespace banjo
