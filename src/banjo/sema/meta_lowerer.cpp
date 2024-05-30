#include "meta_lowerer.hpp"

#include "ast/ast_block.hpp"
#include "ast/ast_child_indices.hpp"
#include "ast/ast_node.hpp"
#include "sema/expr_analyzer.hpp"
#include "sema/meta_evaluator.hpp"
#include "sema/semantic_analyzer.hpp"
#include "sema/semantic_analyzer_context.hpp"
#include "sema/type_analyzer.hpp"
#include "symbol/enumeration.hpp"

namespace lang {

MetaLowerer::MetaLowerer(SemanticAnalyzerContext &context) : context(context) {}

void MetaLowerer::lower(ASTNode *node) {
    for (unsigned int i = 0; i < node->get_children().size(); i++) {
        ASTNode *child = node->get_child(i);
        if (child->get_type() == AST_META_IF) lower_meta_if(child, i);
        else if (child->get_type() == AST_META_FOR) lower_meta_for(child, i);
    }
}

bool MetaLowerer::lower_meta_if(ASTNode *node, unsigned index_in_parent) {
    for (ASTNode *child : node->get_children()) {
        if (child->get_type() == AST_META_IF_CONDITION) {
            Expr *condition_node = child->get_child(IF_CONDITION)->as<Expr>();
            ASTNode *block_node = child->get_child(IF_BLOCK);

            MetaValue value = MetaEvaluator(context).eval_expr(condition_node);
            if (value.result != SemaResult::OK) {
                return false;
            }

            if (value.node->get_type() == AST_TRUE) {
                expand(node->get_parent(), index_in_parent, block_node);
                break;
            }
        } else if (child->get_type() == AST_META_ELSE) {
            ASTNode *block_node = child->get_child();
            expand(node->get_parent(), index_in_parent, block_node);
            break;
        }
    }

    return true;
}

bool MetaLowerer::lower_meta_for(ASTNode *node, unsigned index_in_parent) {
    ASTNode *var_node = node->get_child(0);
    Expr *range_node = node->get_child(1)->as<Expr>();
    ASTBlock *block = node->get_child(2)->as<ASTBlock>();

    unsigned expansion_index = index_in_parent + 1;
    SymbolTable *new_symbol_table = block->get_symbol_table()->get_parent();

    MetaValue iterable = MetaEvaluator(context).eval_expr(range_node);
    if (iterable.result != SemaResult::OK) {
        return false;
    }

    std::vector<ASTNode *> values;
    bool owning_values = false;

    if (iterable.node->get_type() == AST_ARRAY_EXPR) {
        for (ASTNode *child : iterable.node->get_children()) {
            values.push_back(child);
        }

        owning_values = false;
    } else if (iterable.node->get_type() == AST_RANGE) {
        ASTNode *start_node = iterable.node->get_child(0);
        ASTNode *end_node = iterable.node->get_child(1);

        LargeInt start = start_node->as<IntLiteral>()->get_value();
        LargeInt end = end_node->as<IntLiteral>()->get_value();

        for (LargeInt i = start; i < end; i += 1) {
            values.push_back(new IntLiteral(i));
        }

        owning_values = true;
    } else if (iterable.node->get_type() == AST_TUPLE_EXPR) {
        for (ASTNode *child : iterable.node->get_children()) {
            values.push_back(child);
        }

        owning_values = false;
    } else if (iterable.node->get_location() && iterable.node->get_data_type()->get_kind() == DataType::Kind::TUPLE) {
        for (unsigned i = 0; i < iterable.node->get_data_type()->get_tuple().types.size(); i++) {
            Expr *dot_operator = new Expr(AST_DOT_OPERATOR);
            dot_operator->append_child(iterable.node->clone());
            dot_operator->append_child(new IntLiteral(i));
            values.push_back(dot_operator);
        }

        owning_values = true;
    }

    for (ASTNode *expr : values) {
        expr->set_parent(node->get_parent());
        ExprAnalyzer(expr, context).check();

        for (unsigned i = 0; i < block->get_children().size(); i++) {
            ASTNode *child = block->get_child(i);

            ASTNode *clone;
            if (child->get_type() == AST_BLOCK) {
                clone = child->as<ASTBlock>()->clone_block(new_symbol_table);
            } else {
                clone = child->clone();
            }

            replace_identifier_rec(clone, var_node->as<Identifier>()->get_value(), expr);

            node->get_parent()->insert_child(expansion_index, clone);
            expansions.push_back({node->get_parent(), expansion_index});
            expansion_index += 1;
        }
    }

    if (owning_values) {
        for (ASTNode *value : values) {
            delete value;
        }
    }

    return true;
}

bool MetaLowerer::lower_meta_field_access(ASTNode *node) {
    ASTNode *type_node = node->get_child(0);
    ASTNode *field_node = node->get_child(1);

    TypeAnalyzer::Result type_result = TypeAnalyzer(context).analyze(type_node->get_child());
    if (type_result.result != SemaResult::OK) {
        return false;
    }
    DataType *type = type_result.type;

    const std::string &field = field_node->get_value();
    Expr *value;

    if (field == "size") {
        value = new IntLiteral(0);

        // ir::Type ir_type = context.build_type(type);
        // unsigned size = context.get_target()->get_data_layout().get_size(ir_type);
        // value = new IntLiteral(size);
    } else if (field == "name") {
        value = new Expr(AST_STRING_LITERAL, {0, 0}, type->get_structure()->get_name());
    } else if (field == "fields") {
        value = new Expr(AST_ARRAY_EXPR);

        for (StructField *field : type->get_structure()->get_fields()) {
            value->append_child(new Expr(AST_STRING_LITERAL, {0, 0}, field->get_name()));
        }
    } else if (field == "variants") {
        value = new Expr(AST_ARRAY_EXPR);

        for (const auto &it : type->get_enumeration()->get_symbol_table()->get_symbols()) {
            if (it.second.get_kind() == SymbolKind::ENUM_VARIANT) {
                Expr *variant = new Expr(AST_TUPLE_EXPR);
                variant->append_child(new Expr(AST_STRING_LITERAL, {0, 0}, it.first));
                variant->append_child(new IntLiteral(it.second.get_enum_variant()->get_value()));
                value->append_child(variant);
            }
        }
    } else if (field == "is_struct") {
        value = new Expr(type->get_kind() == DataType::Kind::STRUCT ? AST_TRUE : AST_FALSE);
    } else if (field == "is_enum") {
        value = new Expr(type->get_kind() == DataType::Kind::ENUM ? AST_TRUE : AST_FALSE);
    } else if (field == "qualified_name") {
        const std::string &module_name = type->get_structure()->get_module()->get_path().to_string();
        const std::string &struct_name = type->get_structure()->get_name();
        value = new Expr(AST_STRING_LITERAL, {0, 0}, module_name + "." + struct_name);
    } else {
        context.register_error(field_node, ReportText::ID::ERR_INVALID_TYPE_FIELD, field);
        return false;
    }

    node->as<MetaExpr>()->set_value(value);
    value->set_parent(node->get_parent());

    ExprAnalyzer analyzer(value, context);
    analyzer.set_expected_type(expected_type);
    
    if (!analyzer.check()) {
        return false;
    }

    node->as<MetaExpr>()->set_data_type(analyzer.get_type());
    return true;
}

bool MetaLowerer::lower_meta_method_call(ASTNode *node) {
    ASTNode *location_node = node->get_child(CALL_LOCATION);
    ASTNode *type_node = location_node->get_child(0);
    ASTNode *name_node = location_node->get_child(1);
    ASTNode *args_node = node->get_child(CALL_ARGS);

    TypeAnalyzer::Result type_result = TypeAnalyzer(context).analyze(type_node->get_child());
    if (type_result.result != SemaResult::OK) {
        return false;
    }
    DataType *type = type_result.type;

    const std::string &name = name_node->get_value();
    Expr *value;

    if (name == "fieldof") {
        const std::string &field_name = args_node->get_child(1)->get_value();

        value = new Expr(AST_DOT_OPERATOR);
        value->append_child(args_node->get_child(0)->clone());
        value->append_child(new Identifier(field_name, {0, 0}));
    } else if (name == "has_method") {
        if (type->has_methods()) {
            const std::string &method_name = args_node->get_child(0)->get_value();
            value = new Expr(type->get_method_table().get_function(method_name) ? AST_TRUE : AST_FALSE);
        } else {
            value = new Expr(AST_FALSE);
        }
    } else {
        context.register_error(name_node, ReportText::ID::ERR_INVALID_TYPE_METHOD, name);
        return false;
    }

    node->as<MetaExpr>()->set_value(value);
    value->set_parent(node->get_parent());

    ExprAnalyzer analyzer(value, context);
    if (!analyzer.check()) {
        return false;
    }

    node->as<MetaExpr>()->set_data_type(analyzer.get_type());
    return true;
}

void MetaLowerer::expand(ASTNode *parent, unsigned index, ASTNode *block_node) {
    for (unsigned i = 0; !block_node->get_children().empty(); i++) {
        ASTNode *child = block_node->detach_child(0);
        unsigned expansion_index = index + i + 1;
        parent->insert_child(expansion_index, child);
        expansions.push_back({parent, expansion_index});

        context.get_sema().run_name_stage(child);
    }
}

void MetaLowerer::replace_identifier_rec(ASTNode *node, const std::string &identifier, ASTNode *replacement) {
    for (ASTNode *child : node->get_children()) {
        if (child->get_type() == AST_IDENTIFIER && child->as<Identifier>()->get_value() == identifier) {
            node->replace_child(child, replacement->clone(), true);
        } else {
            replace_identifier_rec(child, identifier, replacement);
        }
    }
}

} // namespace lang
