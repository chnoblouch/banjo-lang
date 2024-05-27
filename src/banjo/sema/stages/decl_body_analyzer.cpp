#include "decl_body_analyzer.hpp"

#include "ast/ast_block.hpp"
#include "ast/ast_child_indices.hpp"
#include "ast/ast_node.hpp"
#include "ast/decl.hpp"
#include "sema/block_analyzer.hpp"
#include "sema/expr_analyzer.hpp"
#include "sema/params_analyzer.hpp"
#include "symbol/symbol_ref.hpp"
#include <cassert>

namespace lang {

DeclBodyAnalyzer::DeclBodyAnalyzer(SemanticAnalyzer &sema) : sema(sema), context(sema.get_context()) {}

void DeclBodyAnalyzer::run(Symbol *symbol) {
    ASTNode *node = symbol->get_node();

    switch (node->get_type()) {
        case AST_FUNCTION_DEFINITION: analyze_func(node->as<ASTFunc>()); break;
        case AST_CONSTANT: analyze_const(node->as<ASTConst>()); break;
        case AST_VAR: analyze_var(node->as<ASTVar>()); break;
        case AST_STRUCT_DEFINITION: analyze_struct(node->as<ASTStruct>()); break;
        case AST_UNION: analyze_union(node->as<ASTUnion>()); break;
        default: break;
    }

    node->set_sema_stage(SemaStage::BODY);
}

void DeclBodyAnalyzer::analyze_func(ASTFunc *node) {
    ASTNode *params_node = node->get_child(FUNC_PARAMS);
    ASTBlock *block = node->get_child(FUNC_BLOCK)->as<ASTBlock>();

    Function *func = node->as<lang::ASTFunc>()->get_symbol();
    context.push_ast_context().cur_func = func;

    ParamsAnalyzer(context).analyze(params_node, block);
    BlockAnalyzer(block, node, context).check();

    context.merge_move_scopes_into_parent();
    context.pop_ast_context();
}

void DeclBodyAnalyzer::analyze_const(ASTConst *node) {
    ASTNode *value_node = node->get_child(CONST_VALUE);

    ExprAnalyzer analyzer(value_node, context);
    analyzer.set_expected_type(node->get_symbol()->get_data_type());
    analyzer.check();
}

void DeclBodyAnalyzer::analyze_var(ASTVar *node) {
    if (node->get_children().size() < 3) {
        return;
    }

    ASTNode *value_node = node->get_child(VAR_VALUE);
    ExprAnalyzer(value_node, context).check();
}

void DeclBodyAnalyzer::analyze_struct(ASTStruct *node) {
    ASTNode *proto_impls = node->get_child(STRUCT_IMPL_LIST);
    ASTNode *block = node->get_child(STRUCT_BLOCK);

    for (ASTNode *proto_impl : proto_impls->get_children()) {
        const std::string &name = proto_impl->get_value();
        std::optional<SymbolRef> proto_symbol = context.get_ast_context().get_cur_symbol_table()->get_symbol(name);
        assert(proto_symbol->get_kind() == SymbolKind::PROTO);
        Protocol* proto = proto_symbol->get_proto();
        node->get_symbol()->add_proto_impl(proto);
    }

    context.push_ast_context().enclosing_symbol = node->get_symbol();
    for (ASTNode *child : block->get_children()) {
        sema.run_body_stage(child);
    }
    context.pop_ast_context();
}

void DeclBodyAnalyzer::analyze_union(ASTUnion *node) {
    ASTNode *block = node->get_child(UNION_BLOCK);

    context.push_ast_context().enclosing_symbol = node->get_symbol();
    for (ASTNode *child : block->get_children()) {
        sema.run_body_stage(child);
    }
    context.pop_ast_context();
}

} // namespace lang
