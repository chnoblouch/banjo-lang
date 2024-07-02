#include "decl_body_analyzer.hpp"

#include "ast/ast_block.hpp"
#include "ast/ast_child_indices.hpp"
#include "ast/ast_node.hpp"
#include "ast/decl.hpp"
#include "ast/expr.hpp"
#include "sema/block_analyzer.hpp"
#include "sema/expr_analyzer.hpp"
#include "sema/params_analyzer.hpp"
#include "sema/type_analyzer.hpp"
#include "symbol/data_type.hpp"
#include "symbol/symbol_ref.hpp"

#include <cassert>

namespace banjo {

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
    Structure *struct_ = node->get_symbol();

    ASTNode *name_node = node->get_child(STRUCT_NAME);
    ASTNode *proto_impls = node->get_child(STRUCT_IMPL_LIST);
    ASTNode *block = node->get_child(STRUCT_BLOCK);

    for (ASTNode *proto_impl : proto_impls->get_children()) {
        TypeAnalyzer::Result result = TypeAnalyzer(context).analyze(proto_impl);
        if (result.result != SemaResult::OK) {
            continue;
        }

        if (result.type->get_kind() != DataType::Kind::PROTO) {
            context.register_error(proto_impl, ReportText::ID::ERR_NOT_PROTOCOL, result.type);
            continue;
        }

        Protocol *proto = result.type->get_protocol();
        struct_->add_proto_impl(proto);
    }

    context.push_ast_context().enclosing_symbol = struct_;
    for (ASTNode *child : block->get_children()) {
        sema.run_body_stage(child);
    }
    context.pop_ast_context();

    for (ProtoImpl &proto_impl : struct_->get_proto_impls()) {
        const std::string &proto_name = proto_impl.proto->get_name();

        for (const FunctionSignature &func_signature : proto_impl.proto->get_func_signatures()) {
            const std::string &func_name = func_signature.name;
            Function *method = struct_->get_method_table().get_function(func_name);

            if (!method) {
                context.register_error(name_node, ReportText::ID::ERR_PROTO_IMPL_MISSING_FUNC, func_name, proto_name);
                continue;
            }

            bool signature_matches = true;

            if (method->get_type().param_types.size() == func_signature.type.param_types.size()) {
                for (unsigned i = 1; i < method->get_type().param_types.size(); i++) {
                    if (!method->get_type().param_types[i]->equals(func_signature.type.param_types[i])) {
                        signature_matches = false;
                        break;
                    }
                }

                if (signature_matches && !method->get_type().return_type->equals(func_signature.type.return_type)) {
                    signature_matches = false;
                }
            } else {
                signature_matches = false;
            }

            if (!signature_matches) {
                context.register_error(name_node, ReportText::ID::ERR_SIGNATURE_MISMATCH, func_name, proto_name);
            }
        }
    }
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

} // namespace banjo
