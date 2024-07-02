#include "meta_evaluator.hpp"

#include "banjo/ast/ast_child_indices.hpp"
#include "banjo/ast/ast_node.hpp"
#include "banjo/ast/expr.hpp"
#include "banjo/reports/report_utils.hpp"
#include "banjo/sema/expr_analyzer.hpp"
#include "banjo/sema/location_analyzer.hpp"
#include "banjo/sema/semantic_analyzer.hpp"
#include "banjo/sema/semantic_analyzer_context.hpp"
#include "banjo/sema/type_analyzer.hpp"
#include "banjo/symbol/data_type.hpp"
#include "banjo/utils/macros.hpp"

namespace banjo {

namespace lang {

MetaValue::MetaValue(MetaValue &&other) noexcept
  : result(other.result),
    node(std::exchange(other.node, nullptr)),
    owning(other.owning) {}

MetaValue::~MetaValue() {
    if (owning) {
        delete node;
    }
}

MetaValue &MetaValue::operator=(MetaValue &&other) noexcept {
    result = other.result;
    node = std::exchange(other.node, nullptr);
    owning = other.owning;
    return *this;
}

MetaEvaluator::MetaEvaluator(SemanticAnalyzerContext &context) : context(context) {}

MetaValue MetaEvaluator::eval_expr(Expr *node) {
    context.get_sema().run_body_stage(node);

    switch (node->get_type()) {
        case AST_INT_LITERAL:
        case AST_FLOAT_LITERAL:
        case AST_FALSE:
        case AST_TRUE:
        case AST_STRING_LITERAL:
        case AST_ARRAY_EXPR:
        case AST_TUPLE_EXPR: {
            if (ExprAnalyzer(node, context).check()) {
                return {SemaResult::OK, node, false};
            } else {
                return {SemaResult::ERROR};
            }
        }

        case AST_OPERATOR_ADD:
        case AST_OPERATOR_SUB:
        case AST_OPERATOR_MUL:
        case AST_OPERATOR_DIV:
        case AST_OPERATOR_MOD:
        case AST_OPERATOR_BIT_AND:
        case AST_OPERATOR_BIT_OR:
        case AST_OPERATOR_BIT_XOR:
        case AST_OPERATOR_SHL:
        case AST_OPERATOR_SHR:
        case AST_OPERATOR_EQ:
        case AST_OPERATOR_NE:
        case AST_OPERATOR_GT:
        case AST_OPERATOR_LT:
        case AST_OPERATOR_GE:
        case AST_OPERATOR_LE:
        case AST_OPERATOR_AND:
        case AST_OPERATOR_OR: return eval_operator(node);
        case AST_OPERATOR_NOT: return eval_not(node);
        case AST_IDENTIFIER: return eval_location(node);
        case AST_DOT_OPERATOR: return eval_location(node);
        case AST_EXPLICIT_TYPE: return eval_type(node->as<Expr>());
        case AST_META_FIELD_ACCESS: return eval_location(node);
        case AST_META_METHOD_CALL: return eval_location(node);
        case AST_RANGE: return eval_range(node);

        default: context.register_error("invalid value in meta epression", node); return {SemaResult::ERROR};
    }
}

MetaValue MetaEvaluator::eval_operator(Expr *node) {
    Expr *lhs_node = node->get_child(0)->as<Expr>();
    Expr *rhs_node = node->get_child(1)->as<Expr>();
    ASTNodeType op = node->get_type();

    MetaValue lhs = eval_expr(lhs_node);
    if (lhs.result != SemaResult::OK) {
        return {SemaResult::ERROR};
    }

    if (op == AST_OPERATOR_AND && lhs.node->get_type() != AST_TRUE) {
        return from_bool(false);
    } else if (op == AST_OPERATOR_OR && lhs.node->get_type() == AST_TRUE) {
        return from_bool(true);
    }

    MetaValue rhs = eval_expr(rhs_node);
    if (rhs.result != SemaResult::OK) {
        return {SemaResult::ERROR};
    }

    if (lhs.node->get_type() != AST_EXPLICIT_TYPE && !lhs.node->get_data_type()->equals(rhs.node->get_data_type())) {
        std::string lhs_type = ReportUtils::type_to_string(lhs.node->get_data_type());
        std::string rhs_type = ReportUtils::type_to_string(rhs.node->get_data_type());
        context.register_error("incompatible types in meta expression (" + lhs_type + " and " + rhs_type + ")", node);
        return {SemaResult::ERROR};
    }

    if (op == AST_OPERATOR_EQ) {
        if (lhs.node->get_type() == AST_INT_LITERAL) return from_bool(to_int(lhs) == to_int(rhs));
        else if (lhs.node->get_type() == AST_FLOAT_LITERAL) return from_bool(to_fp(lhs) == to_fp(rhs));
        else if (lhs.node->get_type() == AST_EXPLICIT_TYPE)
            return from_bool(lhs.node->get_data_type()->equals(rhs.node->get_data_type()));
        else ASSERT_UNREACHABLE;
    } else if (op == AST_OPERATOR_NE) {
        if (lhs.node->get_type() == AST_INT_LITERAL) return from_bool(to_int(lhs) != to_int(rhs));
        else if (lhs.node->get_type() == AST_FLOAT_LITERAL) return from_bool(to_fp(lhs) != to_fp(rhs));
        else if (lhs.node->get_type() == AST_EXPLICIT_TYPE)
            return from_bool(!lhs.node->get_data_type()->equals(rhs.node->get_data_type()));
        else ASSERT_UNREACHABLE;
    } else if (op == AST_OPERATOR_AND) {
        return rhs;
    } else if (op == AST_OPERATOR_OR) {
        return rhs;
    } else {
        ASSERT_UNREACHABLE;
    }
}

MetaValue MetaEvaluator::eval_not(Expr *node) {
    Expr *child_node = node->get_child()->as<Expr>();

    MetaValue child = eval_expr(child_node);
    if (child.result != SemaResult::OK) {
        return {SemaResult::ERROR};
    }

    if (child.node->get_type() == AST_FALSE) {
        return from_bool(true);
    } else if (child.node->get_type() == AST_TRUE) {
        return from_bool(false);
    } else {
        ASSERT_UNREACHABLE;
    }
}

MetaValue MetaEvaluator::eval_location(Expr *node) {
    LocationAnalyzer analyzer(node, context);
    SemaResult result = analyzer.check();
    if (result != SemaResult::OK) {
        return {result};
    }

    const LocationElement &back = analyzer.get_location().get_last_element();

    if (back.is_param()) {
        return {SemaResult::OK, node, false};
    } else if (back.is_const()) {
        return eval_expr(back.get_const()->get_node()->get_child(CONST_VALUE)->as<Expr>());
    } else if (back.is_expr()) {
        if (back.get_expr()->get_type() == AST_META_FIELD_ACCESS ||
            back.get_expr()->get_type() == AST_META_METHOD_CALL) {
            return {SemaResult::OK, back.get_expr()->as<MetaExpr>()->get_value(), false};
        } else {
            return eval_expr(back.get_expr());
        }
    } else {
        return {SemaResult::ERROR};
    }
}

LargeInt MetaEvaluator::to_int(const MetaValue &value) {
    return value.node->as<IntLiteral>()->get_value();
}

double MetaEvaluator::to_fp(const MetaValue &value) {
    return std::stod(value.node->get_value());
}

bool MetaEvaluator::to_bool(const MetaValue &value) {
    if (value.node->get_type() == AST_TRUE) {
        return true;
    } else if (value.node->get_type() == AST_FALSE) {
        return false;
    } else {
        ASSERT_UNREACHABLE;
    }
}

MetaValue MetaEvaluator::from_int(LargeInt value) {
    Expr *expr = new IntLiteral(value);
    expr->set_data_type(context.get_type_manager().get_primitive_type(PrimitiveType::I32));
    return {SemaResult::OK, expr, true};
}

MetaValue MetaEvaluator::from_fp(double value) {
    Expr *expr = new Expr(AST_FLOAT_LITERAL, {0, 0}, std::to_string(value));
    expr->set_data_type(context.get_type_manager().get_primitive_type(PrimitiveType::F32));
    return {SemaResult::OK, expr, true};
}

MetaValue MetaEvaluator::from_bool(bool value) {
    Expr *expr = new Expr(value ? AST_TRUE : AST_FALSE);
    expr->set_data_type(context.get_type_manager().get_primitive_type(PrimitiveType::BOOL));
    return {SemaResult::OK, expr, true};
}

MetaSymbol MetaEvaluator::resolve_ident(const std::string &ident, ASTNode *block) {
    for (ASTNode *child : block->get_children()) {
        if (child->get_type() == AST_CONSTANT && child->get_child(CONST_NAME)->get_value() == ident) {
            return {child, child->get_child(CONST_VALUE)->as<Expr>()};
        }
    }

    return {nullptr, nullptr};
}

MetaValue MetaEvaluator::eval_type(Expr *node) {
    TypeAnalyzer::Result result = TypeAnalyzer(context).analyze(node);
    if (result.result == SemaResult::OK) {
        return {SemaResult::OK, node, false};
    } else {
        return {result.result};
    }
}

MetaValue MetaEvaluator::eval_range(Expr *node) {
    Expr *start_node = node->get_child(0)->as<Expr>();
    Expr *end_node = node->get_child(1)->as<Expr>();

    MetaValue start_value = eval_expr(start_node);
    if (start_value.result != SemaResult::OK) {
        return start_value.result;
    }

    MetaValue end_value = eval_expr(end_node);
    if (end_value.result != SemaResult::OK) {
        return end_value.result;
    }

    Expr *evaluated_range_node = new Expr(AST_RANGE);
    evaluated_range_node->append_child(start_value.node->clone());
    evaluated_range_node->append_child(end_value.node->clone());
    return {SemaResult::OK, evaluated_range_node, true};
}

} // namespace lang

} // namespace banjo
