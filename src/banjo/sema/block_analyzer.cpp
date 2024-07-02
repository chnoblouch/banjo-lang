#include "block_analyzer.hpp"

#include "ast/ast_block.hpp"
#include "ast/ast_child_indices.hpp"
#include "ast/ast_node.hpp"
#include "ast/decl.hpp"
#include "ast/expr.hpp"
#include "sema/deinit_analyzer.hpp"
#include "sema/expr_analyzer.hpp"
#include "sema/location_analyzer.hpp"
#include "sema/meta_lowerer.hpp"
#include "sema/semantic_analyzer_context.hpp"
#include "sema/type_analyzer.hpp"
#include "symbol/generics.hpp"
#include "symbol/magic_functions.hpp"
#include "symbol/standard_types.hpp"
#include "symbol/symbol_table.hpp"
#include <vector>

namespace banjo {

namespace lang {

BlockAnalyzer::BlockAnalyzer(ASTBlock *node, ASTNode *parent, SemanticAnalyzerContext &context)
  : node(node),
    context(context),
    move_scope(parent, node, context) {}

bool BlockAnalyzer::check() {
    context.push_ast_context().cur_block = node->as<ASTBlock>();
    context.push_move_scope(&move_scope);

    for (unsigned i = 0; i < node->get_children().size(); i++) {
        ASTNode *child = node->get_child(i);

        switch (child->get_type()) {
            case AST_VAR: analyze_var_decl(child); break;
            case AST_IMPLICIT_TYPE_VAR: analyze_type_infer_var_decl(child); break;
            case AST_ASSIGNMENT: analyze_assign(child); break;
            case AST_ADD_ASSIGN:
            case AST_SUB_ASSIGN:
            case AST_MUL_ASSIGN:
            case AST_DIV_ASSIGN:
            case AST_MOD_ASSIGN:
            case AST_BIT_AND_ASSIGN:
            case AST_BIT_OR_ASSIGN:
            case AST_BIT_XOR_ASSIGN:
            case AST_SHL_ASSIGN:
            case AST_SHR_ASSIGN: analyze_compound_assign(child); break;
            case AST_IF_CHAIN: analyze_if(child); break;
            case AST_SWITCH: analyze_switch(child); break;
            case AST_WHILE: analyze_while(child); break;
            case AST_FOR: analyze_for(child); break;
            case AST_TRY: analyze_try(child); break;
            case AST_FUNCTION_CALL: ExprAnalyzer(child, context).check(); break;
            case AST_FUNCTION_RETURN: analyze_return(child); break;
            case AST_IDENTIFIER:
            case AST_DOT_OPERATOR: analyze_location(child); break;
            case AST_META_IF: MetaLowerer(context).lower_meta_if(child, i); break;
            case AST_META_FOR: MetaLowerer(context).lower_meta_for(child, i); break;
            default: break;
        }
    }

    context.pop_move_scope();
    context.pop_ast_context();
    
    return true;
}

bool BlockAnalyzer::analyze_var_decl(ASTNode *node) {
    Identifier *name_node = node->get_child(VAR_NAME)->as<Identifier>();
    ASTNode *type_node = node->get_child(VAR_TYPE);

    SymbolTable *symbol_table = this->node->as<ASTBlock>()->get_symbol_table();

    if (check_redefinition(symbol_table, name_node)) {
        return false;
    }

    TypeAnalyzer::Result type_result = TypeAnalyzer(context).analyze(type_node);
    if (type_result.result != SemaResult::OK) {
        return false;
    }
    DataType *type = type_result.type;

    bool is_valid = true;

    if (node->get_children().size() > 2) {
        ASTNode *value_node = node->get_child(VAR_VALUE);

        ExprAnalyzer value_checker(value_node, context);
        value_checker.set_expected_type(type);
        if (!value_checker.check()) {
            is_valid = false;
        }
    }

    const std::string &name = name_node->get_value();

    LocalVariable *local = new LocalVariable(node, type, name);
    node->as<ASTVar>()->set_symbol(local);
    name_node->set_symbol(local);
    symbol_table->add_local_variable(local);

    DeinitAnalyzer(context).analyze_local(this->node->as<ASTBlock>(), local);
    return is_valid;
}

bool BlockAnalyzer::analyze_type_infer_var_decl(ASTNode *node) {
    Identifier *name_node = node->get_child(TYPE_INFERRED_VAR_NAME)->as<Identifier>();
    ASTNode *value_node = node->get_child(TYPE_INFERRED_VAR_VALUE);

    SymbolTable *symbol_table = this->node->as<ASTBlock>()->get_symbol_table();

    if (check_redefinition(symbol_table, name_node)) {
        return false;
    }

    ExprAnalyzer value_checker(value_node, context);
    if (!value_checker.check()) {
        return false;
    }

    DataType *data_type = value_checker.get_type();
    if (!data_type) {
        context.register_error("couldn't infer type", node);
    }

    const std::string &name = name_node->get_value();

    LocalVariable *local = new LocalVariable(node, data_type, name);
    node->as<ASTVar>()->set_symbol(local);
    name_node->set_symbol(local);
    symbol_table->add_local_variable(local);

    DeinitAnalyzer(context).analyze_local(this->node->as<ASTBlock>(), local);

    return true;
}

bool BlockAnalyzer::analyze_assign(ASTNode *node) {
    ASTNode *location_node = node->get_child(ASSIGN_LOCATION);
    ASTNode *value_node = node->get_child(ASSIGN_VALUE);

    LocationAnalyzer location_analyzer(location_node, context);
    if (location_analyzer.check() != SemaResult::OK) {
        return false;
    }

    DataType *expected_type = location_analyzer.get_location().get_type();

    ExprAnalyzer value_checker(value_node, context);
    value_checker.set_expected_type(expected_type);
    return value_checker.check();
}

bool BlockAnalyzer::analyze_compound_assign(ASTNode *node) {
    ASTNode *location_node = node->get_child(ASSIGN_LOCATION);
    ASTNode *value_node = node->get_child(ASSIGN_VALUE);

    LocationAnalyzer location_analyzer(location_node, context);
    if (location_analyzer.check() != SemaResult::OK) {
        return false;
    }

    DataType *expected_type = location_analyzer.get_location().get_type();

    ExprAnalyzer value_checker(value_node, context);
    value_checker.set_expected_type(expected_type);
    return value_checker.check();
}

bool BlockAnalyzer::analyze_if(ASTNode *node) {
    for (unsigned i = 0; i < node->get_children().size(); i++) {
        ASTNode *child = node->get_child(i);

        if (child->get_type() == AST_IF || child->get_type() == AST_ELSE_IF) {
            ASTNode *condition_node = child->get_child(IF_CONDITION);
            ExprAnalyzer(condition_node, context).check();

            if (child->has_child(IF_BLOCK)) {
                ASTBlock *block = child->get_child(IF_BLOCK)->as<ASTBlock>();
                BlockAnalyzer(block, child, context).check();
            }
        } else if (child->get_type() == AST_ELSE) {
            ASTBlock *block = child->get_child()->as<ASTBlock>();
            BlockAnalyzer(block, child, context).check();
        }
    }

    context.merge_move_scopes_into_parent();
    return true;
}

bool BlockAnalyzer::analyze_switch(ASTNode *node) {
    ASTNode *value_node = node->get_child(SWITCH_VALUE);
    ASTNode *cases_node = node->get_child(SWITCH_CASES);

    // TODO: deinit analysis

    ExprAnalyzer value_analyzer(value_node, context);
    if (!value_analyzer.check()) {
        return false;
    }

    bool ok = true;

    for (ASTNode *child : cases_node->get_children()) {
        if (child->get_type() == AST_SWITCH_CASE) {
            Identifier *var_name_node = child->get_child(0)->as<Identifier>();
            ASTNode *var_type_node = child->get_child(1);
            ASTBlock *block = child->get_child(2)->as<ASTBlock>();

            const std::string &var_name = var_name_node->get_value();

            TypeAnalyzer::Result type_result = TypeAnalyzer(context).analyze(var_type_node);
            if (type_result.result != SemaResult::OK) {
                ok = false;
                break;
            }

            DataType *type = type_result.type;
            LocalVariable *local = new LocalVariable(var_name_node, type, var_name);
            var_name_node->set_symbol(local);
            block->get_symbol_table()->add_local_variable(local);

            if (!BlockAnalyzer(block, child, context).check()) {
                ok = false;
                break;
            }
        } else if (child->get_type() == AST_SWITCH_DEFAULT_CASE) {
            ASTBlock *block = child->get_child(0)->as<ASTBlock>();

            if (!BlockAnalyzer(block, child, context).check()) {
                ok = false;
                break;
            }
        }
    }

    context.merge_move_scopes_into_parent();
    return ok;
}

bool BlockAnalyzer::analyze_try(ASTNode *node) {
    bool ok = true;

    for (ASTNode *child : node->get_children()) {
        if (child->get_type() == AST_TRY_SUCCESS_CASE) {
            Identifier *var_node = child->get_child(0)->as<Identifier>();
            ASTNode *expr_node = child->get_child(1);
            ASTBlock *block = child->get_child(2)->as<ASTBlock>();

            ExprAnalyzer expr_analyzer(expr_node, context);
            if (!expr_analyzer.check()) {
                ok = false;
                break;
            }

            DataType *var_type;
            if (StandardTypes::is_optional(expr_analyzer.get_type())) {
                var_type = StandardTypes::get_optional_value_type(expr_analyzer.get_type());
            } else if (StandardTypes::is_result(expr_analyzer.get_type())) {
                var_type = StandardTypes::get_result_value_type(expr_analyzer.get_type());
            } else {
                context.register_error(expr_node, ReportText::ID::ERR_CANNOT_UNWRAP, expr_analyzer.get_type());
                ok = false;
                break;
            }

            const std::string &var_name = var_node->get_value();
            LocalVariable *local = new LocalVariable(var_node, var_type, var_name);
            var_node->set_symbol(local);
            block->get_symbol_table()->add_local_variable(local);

            DeinitAnalyzer(context).analyze_local(block->as<ASTBlock>(), local);
            BlockAnalyzer(block, child, context).check();
        } else if (child->get_type() == AST_TRY_ERROR_CASE) {
            Identifier *var_node = child->get_child(0)->as<Identifier>();
            ASTNode *type_node = child->get_child(1);
            ASTBlock *block = child->get_child(2)->as<ASTBlock>();

            TypeAnalyzer::Result type_result = TypeAnalyzer(context).analyze(type_node);
            if (type_result.result != SemaResult::OK) {
                ok = false;
                break;
            }

            DataType *var_type = type_result.type;
            const std::string &var_name = var_node->get_value();
            LocalVariable *local = new LocalVariable(var_node, var_type, var_name);
            var_node->set_symbol(local);
            block->get_symbol_table()->add_local_variable(local);

            DeinitAnalyzer(context).analyze_local(block->as<ASTBlock>(), local);
            BlockAnalyzer(block, child, context).check();
        } else if (child->get_type() == AST_TRY_ELSE_CASE) {
            ASTBlock *block_node = child->get_child(0)->as<ASTBlock>();
            BlockAnalyzer(block_node, child, context).check();
        }
    }

    context.merge_move_scopes_into_parent();
    return ok;
}

bool BlockAnalyzer::analyze_while(ASTNode *node) {
    ASTNode *condition_node = node->get_child(WHILE_CONDITION);
    ASTBlock *block = node->get_child(WHILE_BLOCK)->as<ASTBlock>();

    bool condition_ok = ExprAnalyzer(condition_node, context).check();
    bool block_ok = BlockAnalyzer(block, node, context).check();

    context.merge_move_scopes_into_parent();
    return condition_ok && block_ok;
}

bool BlockAnalyzer::analyze_for(ASTNode *node) {
    ASTNode *iter_type_node = node->get_child(FOR_ITER_TYPE);
    Identifier *var_node = node->get_child(FOR_VAR)->as<Identifier>();
    ASTNode *expr_node = node->get_child(FOR_EXPR);
    ASTBlock *block = node->get_child(FOR_BLOCK)->as<ASTBlock>();

    ExprAnalyzer expr_analyzer(expr_node, context);
    if (!expr_analyzer.check()) {
        return false;
    }

    // TODO: can't iterate over range using pointer

    DataType *var_type;
    if (expr_node->get_type() == AST_RANGE) {
        var_type = context.get_type_manager().get_primitive_type(PrimitiveType::I32);
    } else if (expr_analyzer.get_type()->get_kind() == DataType::Kind::STRUCT) {
        Structure *iterable_struct = expr_analyzer.get_type()->get_structure();
        Function *iter_func = iterable_struct->get_method_table().get_function(MagicFunctions::ITER);
        DataType *iter_type = iter_func->get_type().return_type;

        Structure *iter_struct = iter_type->get_structure();
        Function *next_func = iter_struct->get_method_table().get_function(MagicFunctions::NEXT);
        DataType *next_type = next_func->get_type().return_type;

        if (iter_type_node->get_value() == "*") {
            var_type = next_type;
        } else {
            var_type = next_type->get_base_data_type();
        }
    } else {
        context.register_error(expr_node->get_range())
            .set_message(ReportText(ReportText::ID::ERR_NOT_ITERABLE).format(expr_analyzer.get_type()).str());
        return false;
    }

    const std::string &var_name = var_node->get_value();
    LocalVariable *local = new LocalVariable(var_node, var_type, var_name);
    var_node->set_symbol(local);
    block->get_symbol_table()->add_local_variable(local);

    DeinitAnalyzer(context).analyze_local(block, local);
    BlockAnalyzer(block, node, context).check();

    context.merge_move_scopes_into_parent();
    return true;
}

bool BlockAnalyzer::analyze_return(ASTNode *node) {
    DataType *return_type = context.get_ast_context().cur_func->get_type().return_type;

    if (!node->get_children().empty()) {
        ExprAnalyzer checker(node->get_child(), context);
        checker.set_expected_type(return_type);

        if (!checker.check()) {
            return false;
        }

        // if (!checker.get_type()->equals(return_type)) {
        //     context.register_error(node->get_range())
        //         .set_message(ReportText(ReportText::ID::ERR_RETURN_TYPE_MISMATCH)
        //                          .format(checker.get_type())
        //                          .format(return_type)
        //                          .str());
        //     return false;
        // }
    } else if (!return_type->is_primitive_of_type(PrimitiveType::VOID)) {
        context.register_error(node->get_range()).set_message(ReportText(ReportText::ID::ERR_RETURN_NO_VALUE).str());
        return false;
    }

    return true;
}

bool BlockAnalyzer::analyze_location(ASTNode *node) {
    return LocationAnalyzer(node, context).check() == SemaResult::OK;
}

bool BlockAnalyzer::check_redefinition(SymbolTable *symbol_table, ASTNode *name_node) {
    const std::string name = name_node->get_value();

    std::optional<SymbolRef> existing_symbol = symbol_table->get_symbol(name);
    if (!existing_symbol) {
        return false;
    }

    SymbolKind kind = existing_symbol->get_kind();
    if (kind == SymbolKind::LOCAL || kind == SymbolKind::PARAM) {
        std::string message = ReportText(ReportText::ID::ERR_REDEFINITION).format(name).str();
        context.register_error(message, name_node);
        return true;
    }

    return false;
}

} // namespace lang

} // namespace banjo
