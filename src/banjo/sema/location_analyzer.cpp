#include "location_analyzer.hpp"

#include "ast/ast_child_indices.hpp"
#include "ast/ast_module.hpp"
#include "ast/ast_utils.hpp"
#include "ast/expr.hpp"
#include "reports/report_texts.hpp"
#include "reports/report_utils.hpp"
#include "sema/expr_analyzer.hpp"
#include "sema/function_resolution.hpp"
#include "sema/generics_instantiator.hpp"
#include "sema/meta_lowerer.hpp"
#include "sema/semantic_analyzer.hpp"
#include "sema/type_analyzer.hpp"
#include "symbol/data_type.hpp"
#include "symbol/enumeration.hpp"
#include "symbol/structure.hpp"
#include "symbol/union.hpp"

#include <optional>

namespace lang {

LocationAnalyzer::LocationAnalyzer(ASTNode *node, SemanticAnalyzerContext &context, SymbolUsage usage /* = {} */)
  : node(node),
    context(context),
    usage(usage) {}

SemaResult LocationAnalyzer::check() {
    cur_symbol_table = ASTUtils::find_symbol_table(node);
    is_moving = is_moving_value();

    if (!check(node)) {
        return SemaResult::ERROR;
    }

    if (!location.has_root()) {
        context.register_error(node->get_range()).set_message(ReportText::ERR_EXPECTED_VALUE);
        return SemaResult::ERROR;
    }

    node->as<Expr>()->set_data_type(location.get_type());
    node->as<Expr>()->set_location(location);

    if (is_moving && deinit_info) {
        add_move(deinit_info);
    }

    return SemaResult::OK;
}

bool LocationAnalyzer::check(ASTNode *node) {
    if (node->get_type() == AST_IDENTIFIER) return check_identifier(node);
    else if (node->get_type() == AST_DOT_OPERATOR) return check_dot_operator(node);
    else if (node->get_type() == AST_SELF) return check_self(node);
    else if (node->get_type() == AST_INT_LITERAL) return check_tuple_index(node);
    else if (node->get_type() == AST_GENERIC_INSTANTIATION) return check_generic_instantiation(node);
    else if (node->get_type() == AST_META_FIELD_ACCESS) return check_meta_field_access(node);
    else if (node->get_type() == AST_META_METHOD_CALL) return check_meta_method_call(node);
    else if (node->get_type() == AST_ARRAY_ACCESS) return check_bracket_expr(node->as<BracketExpr>());
    else if (node->get_type() == AST_ERROR) return false;
    else return check_expr(node->as<Expr>());
}

bool LocationAnalyzer::check_identifier(ASTNode *node) {
    if (!location.has_root()) {
        return check_top_level(node);
    }

    DataType *type = location.get_type();
    if (!type) {
        return false;
    }

    if (type->get_kind() == DataType::Kind::ARRAY) {
        // TODO: array members aren't always u64
        DataType *type = context.get_type_manager().get_primitive_type(PrimitiveType::I64);
        location.add_element(LocationElement(node->as<Expr>(), type));
        return true;
    } else if (type->get_kind() == DataType::Kind::STRUCT) {
        return check_struct_member_access(node);
    } else if (type->get_kind() == DataType::Kind::UNION) {
        return check_union_member_access(node);
    } else if (type->get_kind() == DataType::Kind::UNION_CASE) {
        return check_union_case_member_access(node);
    } else if (type->get_kind() == DataType::Kind::POINTER) {
        return check_ptr_member_access(node);
    } else {
        context.register_error(node, ReportText::ERR_TYPE_NO_MEMBERS, type);
        return false;
    }
}

bool LocationAnalyzer::check_struct_member_access(ASTNode *node) {
    const std::string &member_name = node->get_value();
    DataType *type = location.get_type();

    StructField *field = type->get_structure()->get_field(member_name);
    if (field) {
        analyze_args_if_required(field, field->get_type());

        location.add_element(LocationElement(field, field->get_type()));
        context.process_identifier(field, node->as<Identifier>());
        node->as<Expr>()->set_data_type(field->get_type());

        if (is_moving && deinit_info) {
            int index = type->get_structure()->get_field_index(field);
            deinit_info = &deinit_info->member_info[index];
        }

        return true;
    }

    SymbolRef func_symbol = resolve_method(type->get_structure()->get_method_table(), member_name);
    if (func_symbol.get_kind() == SymbolKind::FUNCTION) {
        Function *func = func_symbol.get_func();

        DataType *new_type = context.get_type_manager().new_data_type();
        new_type->set_to_function(func->get_type());
        location.add_element(LocationElement(func, new_type));
        func->set_used(true);
        context.process_identifier(func, node->as<Identifier>());
        return true;
    }

    context.register_error(node, ReportText::ERR_NO_MEMBER, node->get_value(), type);
    return false;
}

bool LocationAnalyzer::check_union_member_access(ASTNode *node) {
    const std::string &member_name = node->get_value();
    DataType *type = location.get_type();

    SymbolRef func_symbol = resolve_method(type->get_union()->get_method_table(), member_name);
    if (func_symbol.get_kind() == SymbolKind::FUNCTION) {
        Function *func = func_symbol.get_func();

        DataType *new_type = context.get_type_manager().new_data_type();
        new_type->set_to_function(func->get_type());
        location.add_element(LocationElement(func, new_type));
        func->set_used(true);
        context.process_identifier(func, node->as<Identifier>());
        return true;
    }

    context.register_error(node, ReportText::ERR_NO_MEMBER, node->get_value(), type);
    return false;
}

bool LocationAnalyzer::check_union_case_member_access(ASTNode *node) {
    const std::string &member_name = node->get_value();
    UnionCase *case_ = location.get_type()->get_union_case();

    std::optional<unsigned> field_index = case_->find_field(member_name);
    if (field_index) {
        UnionCaseField *field = case_->get_field(*field_index);

        location.add_element(LocationElement(field, field->get_type()));
        context.process_identifier(field, node->as<Identifier>());
        node->as<Expr>()->set_data_type(field->get_type());

        if (is_moving && deinit_info) {
            deinit_info = &deinit_info->member_info[*field_index];
        }

        return true;
    }

    context.register_error(node, ReportText::ERR_NO_MEMBER, node->get_value(), location.get_type());
    return false;
}

bool LocationAnalyzer::check_ptr_member_access(ASTNode *node) {
    const std::string &member_name = node->get_value();
    DataType *base_type = location.get_type()->get_base_data_type();

    if (base_type->get_kind() != DataType::Kind::STRUCT && base_type->get_kind() != DataType::Kind::UNION) {
        context.register_error(node, ReportText::ERR_TYPE_NO_MEMBERS, base_type);
        return false;
    }

    if (base_type->get_kind() == DataType::Kind::STRUCT) {
        StructField *field = base_type->get_structure()->get_field(member_name);
        if (field) {
            analyze_args_if_required(field, field->get_type());

            location.add_element(LocationElement(field, field->get_type()));
            context.process_identifier(field, node->as<Identifier>());
            node->as<Expr>()->set_data_type(field->get_type());
            return true;
        }
    }

    SymbolRef func_symbol = resolve_method(base_type->get_method_table(), member_name);
    if (func_symbol.get_kind() == SymbolKind::FUNCTION) {
        Function *func = func_symbol.get_func();

        DataType *new_type = context.get_type_manager().new_data_type();
        new_type->set_to_function(func->get_type());
        location.add_element(LocationElement(func, new_type));
        func->set_used(true);
        context.process_identifier(func, node->as<Identifier>());
        return true;
    }

    context.register_error(node, ReportText::ERR_NO_MEMBER, node->get_value(), base_type);
    return false;
}

bool LocationAnalyzer::check_dot_operator(ASTNode *node) {
    if (!check(node->get_child(0))) {
        return false;
    }

    if (!check(node->get_child(1))) {
        return false;
    }

    return true;
}

bool LocationAnalyzer::check_top_level(ASTNode *node) {
    const std::string &name = node->get_value();

    std::optional<SymbolRef> symbol = context.get_sema().resolve_symbol(cur_symbol_table, name);
    if (!symbol) {
        report_root_not_found(node);
        return false;
    }

    if (cur_symbol_table != ASTUtils::find_symbol_table(node) && symbol->get_visibility() != SymbolVisbility::PUBLIC) {
        context.register_error(node->get_range())
            .set_message(ReportText(ReportText::ID::ERR_PRVATE).format(name).str());
        return false;
    }

    return check_top_level(node, *symbol);
}

bool LocationAnalyzer::check_top_level(ASTNode *node, SymbolRef symbol) {
    SymbolKind kind = symbol.get_kind();

    if (kind == SymbolKind::FUNCTION || kind == SymbolKind::GENERIC_FUNC || kind == SymbolKind::GROUP) {
        symbol = FunctionResolution(context, node, symbol, usage).resolve();
        if (symbol.get_kind() == SymbolKind::NONE) {
            return {};
        }
    }

    context.process_identifier(symbol, node->as<Identifier>());

    if (symbol.get_kind() == SymbolKind::MODULE) {
        ASTModule *module_ = symbol.get_module();
        cur_symbol_table = ASTUtils::get_module_symbol_table(module_);
    } else if (symbol.get_kind() == SymbolKind::FUNCTION) {
        set_root_func(symbol.get_func());
    } else if (symbol.get_kind() == SymbolKind::LOCAL) {
        LocalVariable *local = symbol.get_local();
        node->as<Expr>()->set_data_type(local->get_data_type());
        location.add_element(LocationElement(local, local->get_data_type()));
        analyze_args_if_required(symbol, local->get_data_type());
    } else if (symbol.get_kind() == SymbolKind::PARAM) {
        Parameter *param = symbol.get_param();
        node->as<Expr>()->set_data_type(param->get_data_type());
        location.add_element(LocationElement(param, param->get_data_type()));
        analyze_args_if_required(symbol, param->get_data_type());
    } else if (symbol.get_kind() == SymbolKind::GLOBAL) {
        GlobalVariable *global = symbol.get_global();
        node->as<Expr>()->set_data_type(global->get_data_type());
        location.add_element(LocationElement(global, global->get_data_type()));
        analyze_args_if_required(symbol, global->get_data_type());
    } else if (symbol.get_kind() == SymbolKind::CONST) {
        Constant *const_ = symbol.get_const();
        node->as<Expr>()->set_data_type(const_->get_data_type());
        location.add_element(LocationElement(const_, const_->get_data_type()));
        analyze_args_if_required(symbol, const_->get_data_type());
    } else if (symbol.get_kind() == SymbolKind::STRUCT) {
        cur_symbol_table = symbol.get_struct()->get_symbol_table();
    } else if (symbol.get_kind() == SymbolKind::ENUM) {
        cur_symbol_table = symbol.get_enum()->get_symbol_table();
    } else if (symbol.get_kind() == SymbolKind::ENUM_VARIANT) {
        EnumVariant *variant = symbol.get_enum_variant();
        DataType *type = context.get_type_manager().new_data_type();
        type->set_to_enumeration(variant->get_enum());
        location.add_element(LocationElement(variant, type));
    } else if (symbol.get_kind() == SymbolKind::UNION) {
        cur_symbol_table = symbol.get_union()->get_symbol_table();
    } else if (symbol.get_kind() == SymbolKind::UNION_CASE) {
        UnionCase *case_ = symbol.get_union_case();
        DataType *type = context.get_type_manager().new_data_type();
        type->set_to_union_case(case_);
        location.add_element(LocationElement(case_, type));
    } else if (symbol.get_kind() == SymbolKind::GENERIC_FUNC) {
        location.add_element({symbol.get_generic_func(), nullptr});
    } else if (symbol.get_kind() == SymbolKind::GENERIC_STRUCT) {
        location.add_element(LocationElement(symbol.get_generic_struct(), nullptr));
    } else {
        return false;
    }

    if (location.has_root() && location.get_type() && location.get_type()->get_kind() == DataType::Kind::STRUCT) {
        if (symbol.get_kind() == SymbolKind::LOCAL) {
            deinit_info = &symbol.get_local()->get_deinit_info();
        } else if (symbol.get_kind() == SymbolKind::PARAM) {
            deinit_info = &symbol.get_param()->get_deinit_info();
        }

        if (deinit_info && deinit_info->has_deinit && deinit_info->is_moved) {
            context.register_error(node->get_range())
                .set_message(ReportText(ReportText::ID::ERR_USE_AFTER_MOVE).str())
                .add_note(ReportMessage{
                    ReportText(ReportText::ID::NOTE_USE_AFTER_MOVE_PREVIOUS).str(),
                    {
                        context.get_ast_context().cur_module->get_path(),
                        deinit_info->last_move->get_range(),
                    },
                });

            return false;
        }
    }

    return true;
}

bool LocationAnalyzer::check_self(ASTNode *node) {
    if (location.has_root()) {
        context.register_error("self can't be a member", node);
        return false;
    }

    if (!context.get_ast_context().enclosing_symbol.get_struct()) {
        context.register_error(node, ReportText::ID::ERR_INVALID_SELF);
        return false;
    }

    Structure *struct_ = context.get_ast_context().enclosing_symbol.get_struct();

    DataType *base = context.get_type_manager().new_data_type();
    base->set_to_structure(struct_);
    DataType *type = context.get_type_manager().new_data_type();
    type->set_to_pointer(base);
    location.add_element(LocationElement(struct_, type));
    node->as<Expr>()->set_data_type(type);

    return true;
}

bool LocationAnalyzer::check_tuple_index(ASTNode *node) {
    unsigned index = (unsigned)node->as<IntLiteral>()->get_value().to_u64();
    DataType *type = location.get_type()->get_tuple().types[index];
    location.add_element(LocationElement(index, type));
    return true;
}

bool LocationAnalyzer::check_generic_instantiation(ASTNode *node) {
    ASTNode *template_node = node->get_child(GENERIC_INSTANTIATION_TEMPLATE);
    ASTNode *arg_list = node->get_child(GENERIC_INSTANTIATION_ARGS);

    LocationAnalyzer location_analyzer(template_node, context);
    if (location_analyzer.check() != SemaResult::OK) {
        return false;
    }

    const LocationElement &element = location_analyzer.get_location().get_last_element();

    if (element.is_generic_func()) {
        GenericFunc *generic_func = element.get_generic_func();
        GenericFuncInstance *instance = GenericsInstantiator(context).instantiate_func(generic_func, arg_list);

        Function *func = instance->entity;
        DataType *type = context.get_type_manager().new_data_type();
        type->set_to_function(func->get_type());

        location.add_element(LocationElement(func, type));
        func->set_used(true);

        FunctionResolution(context, node, func, usage).analyze_arg_types();
    } else if (element.is_generic_struct()) {
        GenericStruct *generic_struct = element.get_generic_struct();
        GenericStructInstance *instance = GenericsInstantiator(context).instantiate_struct(generic_struct, arg_list);

        Structure *struct_ = instance->entity;
        cur_symbol_table = struct_->get_symbol_table();
    } else {
        context.register_error("not template in generic instantiation", template_node);
        return false;
    }

    return true;
}

bool LocationAnalyzer::check_bracket_expr(BracketExpr *node) {
    ASTNode *template_node = node->get_child(0);
    ASTNode *arg_list = node->get_child(1);

    LocationAnalyzer location_analyzer(template_node, context);
    if (location_analyzer.check() != SemaResult::OK) {
        return false;
    }

    const LocationElement &element = location_analyzer.get_location().get_last_element();

    if (element.is_generic_func()) {
        GenericFunc *generic_func = element.get_generic_func();
        GenericFuncInstance *instance = GenericsInstantiator(context).instantiate_func(generic_func, arg_list);

        Function *func = instance->entity;
        DataType *type = context.get_type_manager().new_data_type();
        type->set_to_function(func->get_type());

        location.add_element(LocationElement(func, type));
        func->set_used(true);

        FunctionResolution(context, node, func, usage).analyze_arg_types();

        node->set_kind(BracketExpr::Kind::GENERIC_INSTANTIATION);
    } else if (element.is_generic_struct()) {
        GenericStruct *generic_struct = element.get_generic_struct();
        GenericStructInstance *instance = GenericsInstantiator(context).instantiate_struct(generic_struct, arg_list);

        Structure *struct_ = instance->entity;
        cur_symbol_table = struct_->get_symbol_table();

        node->set_kind(BracketExpr::Kind::GENERIC_INSTANTIATION);
    } else {
        return check_expr(node);
    }

    return true;
}

bool LocationAnalyzer::check_expr(Expr *node) {
    ExprAnalyzer checker(node, context);
    if (checker.check()) {
        location.add_element(LocationElement(node, checker.get_type()));
        return true;
    } else {
        return false;
    }
}

void LocationAnalyzer::set_root_func(Function *func) {
    DataType *type = context.get_type_manager().new_data_type();
    type->set_to_function(func->get_type());
    location.add_element(LocationElement(func, type));
    func->set_used(true);
}

void LocationAnalyzer::report_root_not_found(ASTNode *node) {
    context.register_error(node->get_range()).set_message(ReportText(ReportText::ERR_NO_VALUE).format(node).str());
}

void LocationAnalyzer::analyze_args_if_required(SymbolRef symbol, DataType *type) {
    if (!usage.arg_list) {
        return;
    }

    if (type->get_kind() == DataType::Kind::FUNCTION || type->get_kind() == DataType::Kind::CLOSURE) {
        FunctionResolution(context, node, symbol, usage).analyze_arg_types();
    }
}

bool LocationAnalyzer::check_meta_field_access(ASTNode *node) {
    MetaLowerer lowerer(context);
    if (lowerer.lower_meta_field_access(node)) {
        MetaExpr *expr = node->as<MetaExpr>();
        location.add_element(LocationElement(expr, expr->get_data_type()));
        return true;
    } else {
        return false;
    }
}

bool LocationAnalyzer::check_meta_method_call(ASTNode *node) {
    MetaLowerer lowerer(context);
    if (lowerer.lower_meta_method_call(node)) {
        MetaExpr *expr = node->as<MetaExpr>();
        location.add_element(LocationElement(expr, expr->get_data_type()));
        return true;
    } else {
        return false;
    }
}

bool LocationAnalyzer::is_moving_value() {
    switch (node->get_parent()->get_type()) {
        case AST_VAR: return node->get_parent()->get_child(VAR_VALUE) == node;
        case AST_IMPLICIT_TYPE_VAR: return node->get_parent()->get_child(TYPE_INFERRED_VAR_VALUE) == node;
        case AST_ASSIGNMENT: return node->get_parent()->get_child(ASSIGN_VALUE) == node;
        case AST_FUNCTION_RETURN:
        case AST_FUNCTION_ARGUMENT_LIST:
        case AST_STRUCT_FIELD_VALUE:
        case AST_ARRAY_EXPR:
        case AST_MAP_EXPR_PAIR:
        case AST_TUPLE_EXPR: return true;
        default: return false;
    }
}

void LocationAnalyzer::add_move(DeinitInfo *deinit_info) {
    deinit_info->is_moved = true;
    deinit_info->last_move = node;

    if (deinit_info->has_deinit) {
        ASTBlock *cur_block = context.get_ast_context().cur_block;
        cur_block->add_value_move(ValueMove{node, deinit_info});
    }

    for (DeinitInfo &member_info : deinit_info->member_info) {
        add_move(&member_info);
    }
}

SymbolRef LocationAnalyzer::resolve_method(MethodTable &method_table, const std::string &name) {
    std::vector<Function *> funcs = method_table.get_functions(name);
    SymbolRef funcs_symbol;
    SymbolGroup tmp_group;

    if (funcs.size() == 1) {
        funcs_symbol = funcs[0];
    } else if (funcs.size() > 1) {
        for (Function *func : funcs) {
            tmp_group.symbols.push_back(func);
        }

        funcs_symbol = &tmp_group;
    }

    return FunctionResolution(context, node, funcs_symbol, usage).resolve();
}

} // namespace lang
