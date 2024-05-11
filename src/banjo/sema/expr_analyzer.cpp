#include "expr_analyzer.hpp"

#include "ast/ast_block.hpp"
#include "ast/ast_child_indices.hpp"
#include "ast/ast_node.hpp"
#include "ast/ast_utils.hpp"
#include "ast/expr.hpp"
#include "reports/report_texts.hpp"
#include "reports/report_utils.hpp"
#include "sema/block_analyzer.hpp"
#include "sema/generics_instantiator.hpp"
#include "sema/location_analyzer.hpp"
#include "sema/params_analyzer.hpp"
#include "sema/type_analyzer.hpp"
#include "symbol/data_type.hpp"
#include "symbol/generics.hpp"
#include "symbol/magic_functions.hpp"
#include "symbol/standard_types.hpp"

#include <string_view>
#include <unordered_set>

namespace lang {

ExprAnalyzer::ExprAnalyzer(ASTNode *node, SemanticAnalyzerContext &context) : node(node), context(context) {}

bool ExprAnalyzer::check() {
    bool ok;
    switch (node->get_type()) {
        case AST_INT_LITERAL: ok = check_int_literal(); break;
        case AST_FLOAT_LITERAL: ok = check_float_literal(); break;
        case AST_CHAR_LITERAL: ok = check_char_literal(); break;
        case AST_STRING_LITERAL: ok = check_string_literal(); break;
        case AST_ARRAY_EXPR: ok = check_array_literal(); break;
        case AST_MAP_EXPR: ok = check_map_literal(); break;
        case AST_STRUCT_INSTANTIATION: ok = check_struct_literal(); break;
        case AST_ANON_STRUCT_LITERAL: ok = check_anon_struct_literal(); break;
        case AST_TUPLE_EXPR: ok = check_tuple_literal(); break;
        case AST_CLOSURE: ok = check_closure(); break;
        case AST_FALSE: ok = check_false(); break;
        case AST_TRUE: ok = check_true(); break;
        case AST_NULL: ok = check_null(); break;
        case AST_NONE: ok = check_none(); break;
        case AST_UNDEFINED: ok = check_undefined(); break;
        case AST_OPERATOR_ADD:
        case AST_OPERATOR_SUB:
        case AST_OPERATOR_MUL:
        case AST_OPERATOR_DIV:
        case AST_OPERATOR_MOD:
        case AST_OPERATOR_BIT_AND:
        case AST_OPERATOR_BIT_OR:
        case AST_OPERATOR_BIT_XOR:
        case AST_OPERATOR_SHL:
        case AST_OPERATOR_SHR: ok = check_binary_operator(false); break;
        case AST_OPERATOR_EQ:
        case AST_OPERATOR_NE:
        case AST_OPERATOR_GT:
        case AST_OPERATOR_LT:
        case AST_OPERATOR_GE:
        case AST_OPERATOR_LE:
        case AST_OPERATOR_AND:
        case AST_OPERATOR_OR: ok = check_binary_operator(true); break;
        case AST_OPERATOR_NEG: ok = check_operator_neg(); break;
        case AST_OPERATOR_REF: ok = check_operator_ref(); break;
        case AST_STAR_EXPR: ok = check_operator_deref(); break;
        case AST_OPERATOR_NOT: ok = check_operator_not(); break;
        case AST_IDENTIFIER:
        case AST_DOT_OPERATOR:
        case AST_SELF:
        case AST_META_FIELD_ACCESS:
        case AST_META_METHOD_CALL: ok = check_location(); break;
        case AST_FUNCTION_CALL: ok = check_call(); break;
        case AST_ARRAY_ACCESS: ok = check_array_access(); break;
        case AST_CAST: ok = check_cast(); break;
        case AST_RANGE: ok = check_range(); break;
        case AST_META_EXPR: ok = check_meta_expr(); break;
        case AST_ERROR: ok = false; break;
        default:
            context.register_error(node, ReportText::ID::ERR_EXPECTED_VALUE);
            ok = false;
            break;
    }

    if (!ok) {
        return false;
    }

    if (is_implicit_cast()) {
        DataType *cur_expected = expected_type;
        std::vector<DataType *> coercion_chain;

        if (StandardTypes::is_optional(cur_expected) && !StandardTypes::is_optional(type)) {
            coercion_chain.push_back(cur_expected);
            cur_expected = StandardTypes::get_optional_value_type(cur_expected);
        } else if (StandardTypes::is_result(cur_expected) && !StandardTypes::is_result(type)) {
            coercion_chain.push_back(cur_expected);
            cur_expected = StandardTypes::get_result_value_type(cur_expected);
        }

        if (!type) {
            type = context.get_type_manager().get_primitive_type(PrimitiveType::VOID);
        }

        if (cur_expected->get_kind() == DataType::Kind::UNION && type->get_kind() == DataType::Kind::UNION_CASE) {
            coercion_chain.push_back(cur_expected);
        } else if (StandardTypes::is_string(cur_expected) && !StandardTypes::is_string(type)) {
            coercion_chain.push_back(cur_expected);
        }

        coercion_chain.push_back(type);

        Expr *expr = node->as<Expr>();
        expr->set_data_type(expected_type);
        expr->set_coercion_chain(coercion_chain);
        type = expected_type;
        return true;
    }

    node->as<Expr>()->set_data_type(type);

    if (expected_type) {
        return compare_against_expected_type();
    } else {
        return true;
    }
}

bool ExprAnalyzer::check_int_literal() {
    if (expected_type) {
        if (expected_type->get_kind() == DataType::Kind::PRIMITIVE) {
            switch (expected_type->get_primitive_type()) {
                case PrimitiveType::I8:
                case PrimitiveType::I16:
                case PrimitiveType::I32:
                case PrimitiveType::I64:
                case PrimitiveType::U8:
                case PrimitiveType::U16:
                case PrimitiveType::U32:
                case PrimitiveType::U64: type = expected_type; return true;
                default: break;
            }
        } else if (expected_type->get_kind() == DataType::Kind::POINTER) {
            type = expected_type;
            return true;
        }  else if (StandardTypes::is_optional(expected_type)) {
            type = StandardTypes::get_optional_value_type(expected_type);
            return true;
        } else if (StandardTypes::is_result(expected_type)) {
            type = StandardTypes::get_result_value_type(expected_type);
            return true;
        }

        context.register_error(node, ReportText::ERR_INT_LITERAL_TYPE, expected_type);
        return false;
    } else {
        type = context.get_type_manager().get_primitive_type(PrimitiveType::I32);
        return true;
    }
}

bool ExprAnalyzer::check_float_literal() {
    if (expected_type) {
        if (expected_type->get_kind() == DataType::Kind::PRIMITIVE) {
            switch (expected_type->get_primitive_type()) {
                case PrimitiveType::F32:
                case PrimitiveType::F64: type = expected_type; return true;
                default: break;
            }
        } else if (StandardTypes::is_optional(expected_type)) {
            type = StandardTypes::get_optional_value_type(expected_type);
            return true;
        } else if (StandardTypes::is_result(expected_type)) {
            type = StandardTypes::get_result_value_type(expected_type);
            return true;
        }

        context.register_error(node, ReportText::ERR_FLOAT_LITERAL_TYPE, expected_type);
        return false;
    } else {
        type = context.get_type_manager().get_primitive_type(PrimitiveType::F32);
        return true;
    }
}

bool ExprAnalyzer::check_char_literal() {
    if (node->get_value().empty()) {
        context.register_error(ReportText(ReportText::ID::ERR_CHAR_LITERAL_EMPTY).str(), node);
        return false;
    }

    type = context.get_type_manager().get_primitive_type(PrimitiveType::U8);
    return true;
}

bool ExprAnalyzer::check_string_literal() {
    type = context.get_type_manager().new_data_type();
    type->set_to_pointer(context.get_type_manager().get_primitive_type(PrimitiveType::U8));
    return true;
}

bool ExprAnalyzer::check_array_literal() {
    if (node->get_children().empty()) {
        type = expected_type;
        return type;
    }

    DataType *expected_element_type = nullptr;

    if (expected_type) {
        if (expected_type->get_kind() == DataType::Kind::STATIC_ARRAY) {
            expected_element_type = expected_type->get_static_array_type().base_type;
        } else {
            expected_element_type = StandardTypes::get_array_base_type(expected_type);
        }
    }

    DataType *first_type = nullptr;
    for (ASTNode *child : node->get_children()) {
        // Check the element expression
        ExprAnalyzer element_checker(child, context);
        if (expected_element_type) {
            element_checker.set_expected_type(expected_element_type);
        }

        if (!element_checker.check()) {
            return false;
        }

        // Compare the element type to the first element type
        if (!first_type) {
            first_type = element_checker.get_type();
        } else if (!element_checker.get_type()->equals(first_type)) {
            context.register_error("mixed value types in array literal", node);
            return false;
        }
    }

    if (expected_type) {
        type = expected_type;
    } else {
        type = instantiate_std_generic_struct({"std", "array"}, "Array", {first_type});
    }

    return true;
}

bool ExprAnalyzer::check_map_literal() {
    DataType *key_type = nullptr;
    DataType *value_type = nullptr;

    for (ASTNode *child : node->get_children()) {
        ASTNode *key_node = child->get_child(MAP_LITERAL_PAIR_KEY);
        ASTNode *value_node = child->get_child(MAP_LITERAL_PAIR_VALUE);

        ExprAnalyzer key_checker(key_node, context);
        if (!key_checker.check()) {
            return false;
        }

        ExprAnalyzer value_checker(value_node, context);
        if (!value_checker.check()) {
            return false;
        }

        if (!key_type) {
            key_type = key_checker.get_type();
            value_type = value_checker.get_type();
        }
    }

    type = instantiate_std_generic_struct({"std", "map"}, "Map", {key_type, value_type});
    return true;
}

bool ExprAnalyzer::check_struct_literal() {
    ASTNode *name_node = node->get_child(STRUCT_LITERAL_NAME);
    ASTNode *values_node = node->get_child(STRUCT_LITERAL_VALUES);
    SymbolTable *symbol_table = ASTUtils::find_symbol_table(node);

    Structure *struct_ = nullptr;
    ModulePath path;

    if (name_node->get_type() != AST_ARRAY_ACCESS) {
        TypeAnalyzer::Result type_result = TypeAnalyzer(context).analyze(name_node);
        if (type_result.result != SemaResult::OK) {
            return false;
        }
        type = type_result.type;

        struct_ = type->get_structure();

        if (!struct_) {
            context.register_error(name_node->get_range())
                .set_message(ReportText(ReportText::ID::ERR_NO_TYPE).format(path).str());
            return false;
        }
    } else {
        ASTNode *template_node = name_node->get_child(GENERIC_INSTANTIATION_TEMPLATE);
        ASTNode *arg_list = name_node->get_child(GENERIC_INSTANTIATION_ARGS);

        path = ASTUtils::get_path_from_node(template_node);
        GenericStruct *generic_struct = ASTUtils::resolve_generic_struct(path, symbol_table);
        if (!generic_struct) {
            context.register_error(template_node->get_range()).set_message("this is not a generic struct");
            return false;
        }

        GenericStructInstance *instance = GenericsInstantiator(context).instantiate_struct(generic_struct, arg_list);

        struct_ = instance->entity;
        type = context.get_type_manager().new_data_type();
        type->set_to_structure(struct_);
    }

    return check_any_struct_literal(struct_, values_node);
}

bool ExprAnalyzer::check_anon_struct_literal() {
    ASTNode *values_node = node->get_child();

    if (!expected_type) {
        context.register_error(node->get_range()).set_message(ReportText::ID::ERR_CANNOT_INFER_STRUCT_LITERAL_TYPE);
        return false;
    }

    type = expected_type;
    Structure *struct_ = type->get_structure();

    return check_any_struct_literal(struct_, values_node);
}

bool ExprAnalyzer::check_tuple_literal() {
    std::vector<DataType *> types;
    for (unsigned i = 0; i < node->get_children().size(); i++) {
        ASTNode *child = node->get_child(i);

        ExprAnalyzer child_checker(child, context);
        if (expected_type && expected_type->get_kind() == DataType::Kind::TUPLE) {
            child_checker.set_expected_type(expected_type->get_tuple().types[i]);
        }

        if (!child_checker.check()) {
            return false;
        }

        types.push_back(child_checker.get_type());
    }

    type = context.get_type_manager().new_data_type();
    type->set_to_tuple({types});
    return true;
}

bool ExprAnalyzer::check_closure() {
    ClosureExpr *closure_expr = node->as<ClosureExpr>();

    ASTNode *params_node = node->get_child(CLOSURE_PARAMS);
    ASTNode *return_type_node = node->get_child(CLOSURE_RETURN_TYPE);
    ASTBlock *block = node->get_child(CLOSURE_BLOCK)->as<ASTBlock>();

    std::vector<DataType *> param_types;

    for (ASTNode *child : params_node->get_children()) {
        TypeAnalyzer::Result param_result = TypeAnalyzer(context).analyze_param(child);
        if (param_result.result != SemaResult::OK) {
            return false;
        }
        DataType *param_type = param_result.type;

        param_types.push_back(param_type);
    }

    TypeAnalyzer::Result return_result = TypeAnalyzer(context).analyze(return_type_node);
    if (return_result.result != SemaResult::OK) {
        return false;
    }
    DataType *return_type = return_result.type;

    FunctionType func_type{param_types, return_type};

    FunctionType func_type_with_context = func_type;
    DataType *context_ptr_lang_type = context.get_type_manager().get_primitive_type(PrimitiveType::ADDR);
    func_type_with_context.param_types.insert(func_type_with_context.param_types.begin(), context_ptr_lang_type);

    Function *enclosing_func = context.get_ast_context().cur_func;
    Function *func = new Function(node, "", enclosing_func->get_module());
    func->get_type() = func_type_with_context;
    func->set_ir_func(new ir::Function());
    closure_expr->set_func(func);

    context.push_ast_context().cur_func = func;

    ParamsAnalyzer(context).analyze(params_node, block);
    BlockAnalyzer(block, node, context).check();

    context.merge_move_scopes_into_parent();
    context.pop_ast_context();

    type = context.get_type_manager().new_data_type();
    type->set_to_closure(func_type);
    return true;
}

bool ExprAnalyzer::check_false() {
    type = context.get_type_manager().get_primitive_type(PrimitiveType::BOOL);
    return true;
}

bool ExprAnalyzer::check_true() {
    type = context.get_type_manager().get_primitive_type(PrimitiveType::BOOL);
    return true;
}

bool ExprAnalyzer::check_null() {
    if (!expected_type) {
        type = context.get_type_manager().get_primitive_type(PrimitiveType::ADDR);
    } else {
        type = expected_type;
    }

    return true;
}

bool ExprAnalyzer::check_none() {
    Expr *expr = node->as<Expr>();
    expr->set_data_type(expected_type);
    expr->set_coercion_chain({nullptr});

    return true;
}

bool ExprAnalyzer::check_undefined() {
    type = expected_type;
    return true;
}

bool ExprAnalyzer::check_binary_operator(bool is_bool) {
    Expr *lhs = node->get_child(0)->as<Expr>();
    Expr *rhs = node->get_child(1)->as<Expr>();
    ASTNodeType op = node->get_type();

    ExprAnalyzer lhs_analyzer(lhs, context);
    ExprAnalyzer rhs_analyzer(rhs, context);

    bool lhs_ok = false;
    bool rhs_ok = false;

    if (is_coercible_in_binary_operator(lhs->get_type())) {
        rhs_ok = rhs_analyzer.check();
        if (rhs_ok) {
            if (rhs_analyzer.get_type()->get_kind() != DataType::Kind::STRUCT) {
                lhs_analyzer.set_expected_type(rhs_analyzer.get_type());
            }

            lhs_ok = lhs_analyzer.check();
        }
    } else if (is_coercible_in_binary_operator(rhs->get_type())) {
        lhs_ok = lhs_analyzer.check();
        if (lhs_ok) {
            if (lhs_analyzer.get_type()->get_kind() != DataType::Kind::STRUCT) {
                rhs_analyzer.set_expected_type(lhs_analyzer.get_type());
            }

            rhs_ok = rhs_analyzer.check();
        }
    } else {
        lhs_ok = lhs_analyzer.check();
        rhs_ok = rhs_analyzer.check();
    }

    if (lhs_ok && rhs_ok) {
        if (lhs_analyzer.get_type()->get_kind() == DataType::Kind::STRUCT) {
            Function *operator_func = resolve_operator_overload(op, lhs, rhs);
            node->as<OperatorExpr>()->set_operator_func(operator_func);
            type = operator_func->get_type().return_type;
            return true;
        }

        if (!lhs_analyzer.get_type()->equals(rhs_analyzer.get_type())) {
            std::string message = "non-matching types as operands to binary operator";
            message += " (" + ReportUtils::type_to_string(lhs_analyzer.get_type()) + " and ";
            message += ReportUtils::type_to_string(rhs_analyzer.get_type()) + ")";
            context.register_error(message, node);
            return false;
        }

        if (is_bool) {
            type = context.get_type_manager().get_primitive_type(PrimitiveType::BOOL);
            return true;
        }

        type = lhs_analyzer.get_type();
        return true;
    } else {
        return false;
    }
}

bool ExprAnalyzer::check_operator_neg() {
    ExprAnalyzer child_checker(node->get_child(), context);
    child_checker.set_expected_type(expected_type);
    if (!child_checker.check()) {
        return false;
    }

    type = child_checker.get_type();

    if (child_checker.get_type()->is_unsigned_int()) {
        context.register_error(node, ReportText::ID::ERR_NEGATE_UNSIGNED, type);
        return false;
    }

    return true;
}

bool ExprAnalyzer::check_operator_ref() {
    ExprAnalyzer child_checker(node->get_child(), context);

    if (expected_type && expected_type->get_kind() == DataType::Kind::POINTER) {
        child_checker.set_expected_type(expected_type->get_base_data_type());
    }

    if (!child_checker.check()) {
        return false;
    }

    type = context.get_type_manager().new_data_type();
    type->set_to_pointer(child_checker.get_type());
    return true;
}

bool ExprAnalyzer::check_operator_deref() {
    ExprAnalyzer child_checker(node->get_child(), context);
    if (!child_checker.check()) {
        return false;
    }

    if (child_checker.get_type()->get_kind() == DataType::Kind::POINTER) {
        type = child_checker.get_type()->get_base_data_type();
    } else if (child_checker.get_type()->get_kind() == DataType::Kind::STRUCT) {
        Structure *struct_ = child_checker.get_type()->get_structure();
        Function *func = struct_->get_method_table().get_function("deref");
        type = func->get_type().return_type;
    } else {
        context.register_error(node, ReportText::ID::ERR_CANNOT_DEREFERENCE, child_checker.get_type());
        return false;
    }

    return true;
}

bool ExprAnalyzer::check_operator_not() {
    ExprAnalyzer child_checker(node->get_child(), context);
    if (!child_checker.check()) {
        return false;
    }

    type = context.get_type_manager().get_primitive_type(PrimitiveType::BOOL);
    return true;
}

bool ExprAnalyzer::check_location() {
    LocationAnalyzer location_analyzer(node, context);
    if (location_analyzer.check() == SemaResult::OK) {
        type = location_analyzer.get_location().get_type();
        return true;
    } else {
        return false;
    }
}

bool ExprAnalyzer::check_call() {
    ASTNode *args_node = node->get_child(CALL_ARGS);
    ASTNode *location_node = node->get_child(CALL_LOCATION);

    LocationAnalyzer location_analyzer(location_node, context, args_node);
    if (location_analyzer.check() != SemaResult::OK) {
        return false;
    }

    const Location &location = location_analyzer.get_location();
    if (location.get_last_element().is_union_case()) {
        return check_union_case_expr(location);
    } else {
        return check_function_call(location);
    }
}

bool ExprAnalyzer::check_union_case_expr(const Location &location) {
    ASTNode *args_node = node->get_child(CALL_ARGS);

    for (ASTNode *arg_node : args_node->get_children()) {
        if (!ExprAnalyzer(arg_node, context).check()) {
            return false;
        }
    }

    type = location.get_type();
    return true;
}

bool ExprAnalyzer::check_function_call(const Location &location) {
    ASTNode *args_node = node->get_child(CALL_ARGS);

    Function *func = location.get_func();

    // HACK: Move this into IR Builder
    if (func && func->get_generic_instance()) {
        GenericFuncInstance *generic_instance = func->get_generic_instance();
        GenericFunc *generic_func = generic_instance->generic_entity;

        if (GenericsUtils::has_sequence(generic_func)) {
            Expr *sequence_tuple = new Expr(AST_TUPLE_EXPR);
            sequence_tuple->set_data_type(generic_instance->args.back());

            unsigned start_index = func->get_type().param_types.size() - 1;
            for (unsigned i = start_index; i < args_node->get_children().size(); i++) {
                sequence_tuple->append_child(args_node->get_child(i));
            }

            unsigned num_sequence_args = args_node->get_children().size() - start_index;
            for (unsigned i = 0; i < num_sequence_args; i++) {
                args_node->detach_child(args_node->get_children().size() - 1);
            }

            args_node->append_child(sequence_tuple);
        }
    }

    type = location.get_type()->get_function_type().return_type;
    return true;
}

bool ExprAnalyzer::check_array_access() {
    LocationAnalyzer location_analyzer(node->get_child(0), context);
    if (location_analyzer.check() != SemaResult::OK) {
        return false;
    }

    if (location_analyzer.get_location().get_last_element().is_generic_struct()) {
        return LocationAnalyzer(node, context).check() == SemaResult::OK;
    }

    ExprAnalyzer index_checker(node->get_child(1)->get_child(0), context);

    DataType *location_type = location_analyzer.get_location().get_type();
    if (location_type->get_kind() == DataType::Kind::POINTER) {
        type = location_type->get_base_data_type();
    } else if (location_type->get_kind() == DataType::Kind::STATIC_ARRAY) {
        type = location_type->get_static_array_type().base_type;
    } else if (location_type->get_kind() == DataType::Kind::STRUCT) {
        Function *ref_func = location_type->get_structure()->get_method_table().get_function("ref");
        ref_func->set_used(true);
        type = ref_func->get_type().return_type->get_base_data_type();
        index_checker.set_expected_type(ref_func->get_type().param_types[1]);
    }

    if (!index_checker.check()) {
        return false;
    }

    node->as<BracketExpr>()->set_kind(BracketExpr::Kind::INDEX);
    return true;
}

bool ExprAnalyzer::check_cast() {
    ExprAnalyzer child_checker(node->get_child(0), context);
    if (!child_checker.check()) {
        return false;
    }

    TypeAnalyzer::Result type_result = TypeAnalyzer(context).analyze(node->get_child(1));
    if (type_result.result != SemaResult::OK) {
        return false;
    }
    type = type_result.type;

    return true;
}

bool ExprAnalyzer::check_range() {
    if (!ExprAnalyzer(node->get_child(0), context).check()) {
        return false;
    }

    if (!ExprAnalyzer(node->get_child(1), context).check()) {
        return false;
    }

    type = context.get_type_manager().get_primitive_type(PrimitiveType::VOID);
    return true;
}

bool ExprAnalyzer::check_meta_expr() {
    ASTNode *args_node = node->get_child(META_EXPR_ARGS);

    TypeAnalyzer(context).analyze(args_node->get_child(0));
    type = context.get_type_manager().get_primitive_type(PrimitiveType::U64);
    return true;
}

DataType *ExprAnalyzer::instantiate_std_generic_struct(
    const ModulePath &path,
    std::string name,
    GenericInstanceArgs args
) {
    ASTNode *module_node = context.get_module_manager().get_module_list().get_by_path(path);
    SymbolTable *module_symbol_table = ASTUtils::get_module_symbol_table(module_node);
    GenericStruct *generic_struct = module_symbol_table->get_generic_struct(name);
    GenericStructInstance *instance = GenericsInstantiator(context).instantiate_struct(generic_struct, args);

    DataType *type = context.get_type_manager().new_data_type();
    type->set_to_structure(instance->entity);
    return type;
}

bool ExprAnalyzer::check_any_struct_literal(Structure *struct_, ASTNode *values_node) {
    node->as<Expr>()->set_data_type(type);

    std::unordered_set<std::string_view> assigned_fields;

    for (ASTNode *value : values_node->get_children()) {
        if (value->get_type() != AST_STRUCT_FIELD_VALUE) {
            continue;
        }

        ASTNode *field_name_node = value->get_child(STRUCT_FIELD_VALUE_NAME);
        ASTNode *field_value_node = value->get_child(STRUCT_FIELD_VALUE_VALUE);

        std::string_view field_name = field_name_node->get_value();
        assigned_fields.insert(field_name);

        StructField *field = struct_->get_field(field_name);
        if (!field) {
            context.register_error(field_name_node, ReportText::ID::ERR_NO_MEMBER, field_name_node, type);
            return false;
        }

        ExprAnalyzer value_checker(field_value_node, context);
        value_checker.set_expected_type(field->get_type());
        if (!value_checker.check()) {
            return false;
        }
    }

    std::vector<std::string_view> unassigned_fields;
    for (StructField *field : struct_->get_fields()) {
        if (!assigned_fields.contains(field->get_name())) {
            unassigned_fields.push_back(field->get_name());
        }
    }

    if (!unassigned_fields.empty()) {
        Report &report = context.register_warning(node->get_range());
        report.set_message(ReportText(ReportText::ID::WARN_MISSING_FIELD).format(struct_->get_name()).str());

        for (std::string_view field_name : unassigned_fields) {
            report.add_note({"missing field '" + std::string{field_name} + "'"});
        }
    }

    return true;
}

Function *ExprAnalyzer::resolve_operator_overload(ASTNodeType op, Expr *lhs, Expr *rhs) {
    Structure *struct_ = lhs->get_data_type()->get_structure();
    std::string func_name = MagicFunctions::get_operator_func(op);
    std::vector<Function *> funcs = struct_->get_method_table().get_functions(func_name);

    if (funcs.size() == 1) {
        return funcs[0];
    }

    for (Function *func : funcs) {
        if (func->get_type().param_types[1]->equals(rhs->get_data_type())) {
            return func;
        }
    }

    return nullptr;
}

bool ExprAnalyzer::is_implicit_cast() {
    if (!expected_type) {
        return false;
    }

    if (!type) {
        return true;
    }

    if (StandardTypes::is_optional(expected_type)) {
        return !StandardTypes::is_optional(type);
    } else if (StandardTypes::is_result(expected_type)) {
        return !StandardTypes::is_result(type);
    } else if (StandardTypes::is_string(expected_type)) {
        return !StandardTypes::is_string(type);
    } else if (expected_type->get_kind() == DataType::Kind::UNION) {
        return type->get_kind() == DataType::Kind::UNION_CASE;
    } else {
        return false;
    }
}

bool ExprAnalyzer::compare_against_expected_type() {
    if (expected_type->equals(type)) {
        return true;
    }

    context.register_error(node, ReportText::ID::ERR_TYPE_MISMATCH, expected_type, type);
    return false;
}

bool ExprAnalyzer::is_coercible_in_binary_operator(ASTNodeType type) {
    return type == AST_INT_LITERAL || type == AST_FLOAT_LITERAL || type == AST_NULL;
}

bool ExprAnalyzer::is_std_string_expected() {
    if (!is_struct_expected()) {
        return false;
    }

    Structure *struct_ = expected_type->get_structure();
    const ModulePath &module_path = struct_->get_module()->get_path();
    return struct_->get_name() == "String" && module_path == ModulePath{"std", "string"};
}

bool ExprAnalyzer::is_struct_expected() {
    return expected_type && expected_type->get_kind() == DataType::Kind::STRUCT;
}

} // namespace lang
