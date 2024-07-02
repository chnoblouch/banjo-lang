#include "type_analyzer.hpp"

#include "banjo/ast/ast_child_indices.hpp"
#include "banjo/ast/ast_node.hpp"
#include "banjo/ast/ast_utils.hpp"
#include "banjo/ast/expr.hpp"
#include "banjo/sema/generics_instantiator.hpp"
#include "banjo/sema/semantic_analyzer.hpp"
#include "banjo/sema/semantic_analyzer_context.hpp"
#include "banjo/symbol/symbol_ref.hpp"
#include "banjo/utils/macros.hpp"

namespace banjo {

namespace lang {

TypeAnalyzer::TypeAnalyzer(SemanticAnalyzerContext &context) : context(context) {}

TypeAnalyzer::Result TypeAnalyzer::analyze(ASTNode *node) {
    switch (node->get_type()) {
        case AST_I8: return analyze_primitive_type(node, PrimitiveType::I8);
        case AST_I16: return analyze_primitive_type(node, PrimitiveType::I16);
        case AST_I32: return analyze_primitive_type(node, PrimitiveType::I32);
        case AST_I64: return analyze_primitive_type(node, PrimitiveType::I64);
        case AST_U8: return analyze_primitive_type(node, PrimitiveType::U8);
        case AST_U16: return analyze_primitive_type(node, PrimitiveType::U16);
        case AST_U32: return analyze_primitive_type(node, PrimitiveType::U32);
        case AST_U64: return analyze_primitive_type(node, PrimitiveType::U64);
        case AST_F32: return analyze_primitive_type(node, PrimitiveType::F32);
        case AST_F64: return analyze_primitive_type(node, PrimitiveType::F64);
        case AST_USIZE: return analyze_primitive_type(node, PrimitiveType::U64);
        case AST_BOOL: return analyze_primitive_type(node, PrimitiveType::BOOL);
        case AST_ADDR: return analyze_primitive_type(node, PrimitiveType::ADDR);
        case AST_VOID: return analyze_primitive_type(node, PrimitiveType::VOID);
        case AST_ARRAY_EXPR: return analyze_array_type(node);
        case AST_MAP_EXPR: return analyze_map_type(node);
        case AST_OPTIONAL_DATA_TYPE: return analyze_optional_type(node);
        case AST_RESULT_TYPE: return analyze_result_type(node);
        case AST_IDENTIFIER: return analyze_ident_type(node);
        case AST_DOT_OPERATOR: return analyze_dot_operator_type(node);
        case AST_STAR_EXPR: return analyze_ptr_type(node);
        case AST_FUNCTION_DATA_TYPE: return analyze_func_type(node);
        case AST_STATIC_ARRAY_TYPE: return analyze_static_array_type(node);
        case AST_TUPLE_EXPR: return analyze_tuple_type(node);
        case AST_CLOSURE_TYPE: return analyze_closure_type(node);
        case AST_ARRAY_ACCESS: return analyze_generic_instance_type(node);
        case AST_EXPLICIT_TYPE: return analyze_explicit_type(node);
        case AST_ERROR: return SemaResult::ERROR;
        default: assert(!"type analyzer: not a type node"); return SemaResult::ERROR;
    }
}

TypeAnalyzer::Result TypeAnalyzer::analyze_param(ASTNode *node) {
    ASTNode *name_node = node->get_child(PARAM_NAME);

    if (name_node->get_type() != AST_SELF) {
        ASTNode *type_node = node->get_child(PARAM_TYPE);
        return analyze(type_node);
    } else {
        SymbolKind enclosing_symbol_kind = context.get_ast_context().enclosing_symbol.get_kind();

        if (enclosing_symbol_kind != SymbolKind::STRUCT && enclosing_symbol_kind != SymbolKind::UNION) {
            context.register_error(name_node, ReportText::ID::ERR_INVALID_SELF_PARAM);
            return SemaResult::ERROR;
        }

        DataType *struct_type = context.get_type_manager().new_data_type();
        struct_type->set_to_structure(context.get_ast_context().enclosing_symbol.get_struct());

        DataType *param_type = context.get_type_manager().new_data_type();
        param_type->set_to_pointer(struct_type);

        return param_type;
    }
}

TypeAnalyzer::Result TypeAnalyzer::analyze_primitive_type(ASTNode *node, PrimitiveType primitive) {
    DataType *type = context.get_type_manager().get_primitive_type(primitive);
    node->as<Expr>()->set_data_type(type);
    return type;
}

TypeAnalyzer::Result TypeAnalyzer::analyze_array_type(ASTNode *node) {
    if (node->get_children().empty()) {
        context.register_error(node, ReportText::ID::ERR_INVALID_TYPE);
        return SemaResult::ERROR;
    }

    Result base_result = analyze(node->get_child(0));
    if (base_result.result != SemaResult::OK) {
        return base_result.result;
    }
    DataType *base_type = base_result.type;

    return instantiate_generic_std_type(node, {"std", "array"}, "Array", {base_type});
}

TypeAnalyzer::Result TypeAnalyzer::analyze_map_type(ASTNode *node) {
    if (node->get_children().empty()) {
        context.register_error(node, ReportText::ID::ERR_INVALID_TYPE);
        return SemaResult::ERROR;
    }

    ASTNode *pair_node = node->get_child(0);

    Result key_result = analyze(pair_node->get_child(0));
    if (key_result.result != SemaResult::OK) {
        return key_result.result;
    }
    DataType *key_type = key_result.type;

    Result value_result = analyze(pair_node->get_child(1));
    if (value_result.result != SemaResult::OK) {
        return value_result.result;
    }
    DataType *value_type = value_result.type;

    return instantiate_generic_std_type(node, {"std", "map"}, "Map", {key_type, value_type});
}

TypeAnalyzer::Result TypeAnalyzer::analyze_optional_type(ASTNode *node) {
    Result base_result = analyze(node->get_child());
    if (base_result.result != SemaResult::OK) {
        return base_result.result;
    }
    DataType *base_type = base_result.type;

    return instantiate_generic_std_type(node, {"std", "optional"}, "Optional", {base_type});
}

TypeAnalyzer::Result TypeAnalyzer::analyze_result_type(ASTNode *node) {
    Result success_result = analyze(node->get_child(0));
    if (success_result.result != SemaResult::OK) {
        return success_result.result;
    }
    DataType *success_type = success_result.type;

    Result error_result = analyze(node->get_child(1));
    if (error_result.result != SemaResult::OK) {
        return error_result.result;
    }
    DataType *error_type = error_result.type;

    return instantiate_generic_std_type(node, {"std", "result"}, "Result", {success_type, error_type});
}

TypeAnalyzer::Result TypeAnalyzer::analyze_ident_type(ASTNode *node) {
    const std::string &name = node->get_value();

    if (context.get_ast_context().is_generic_type(name)) {
        DataType *type = context.get_ast_context().get_generic_type_value(name);
        node->as<Expr>()->set_data_type(type);
        return type;
    }

    SymbolTable *symbol_table = ASTUtils::find_symbol_table(node);
    std::optional<SymbolRef> symbol = context.get_sema().resolve_symbol(symbol_table, name);
    if (!symbol) {
        context.register_error(node, ReportText::ID::ERR_NO_TYPE, name);
        return SemaResult::ERROR;
    }

    context.process_identifier(*symbol, node->as<Identifier>());

    DataType *type = from_symbol(*symbol);
    if (!type) {
        return SemaResult::ERROR;
    }

    node->as<Expr>()->set_data_type(type);
    return type;
}

TypeAnalyzer::Result TypeAnalyzer::analyze_dot_operator_type(ASTNode *node) {
    ASTNode *left_node = node->get_child(0);
    ASTNode *right_node = node->get_child(1);

    SymbolTable *symbol_table = ASTUtils::find_symbol_table(node);
    resolve_dot_operator_element(left_node, symbol_table);

    if (!symbol_table) {
        context.register_error("symbol not found", node); // TODO: better error message
        return SemaResult::ERROR;
    }

    const std::string &name = right_node->get_value();
    std::optional<SymbolRef> symbol = context.get_sema().resolve_symbol(symbol_table, name);

    if (symbol) {
        right_node->as<Identifier>()->set_symbol(*symbol);
        // } else if (context.get_incomplete_symbol_tables().contains(symbol_table)) {
        //     return SemaResult::INCOMPLETE;
    } else {
        context.register_error("symbol not found", node); // TODO: better error message
        return SemaResult::ERROR;
    }

    DataType *type = from_symbol(*symbol);
    if (!type) {
        return SemaResult::ERROR;
    }

    node->as<Expr>()->set_data_type(type);
    return type;
}

TypeAnalyzer::Result TypeAnalyzer::analyze_ptr_type(ASTNode *node) {
    Result base_result = analyze(node->get_child());
    if (base_result.result != SemaResult::OK) {
        return base_result.result;
    }
    DataType *base_type = base_result.type;

    DataType *type = context.get_type_manager().new_data_type();
    type->set_to_pointer(base_type);
    node->as<Expr>()->set_data_type(type);
    return type;
}

TypeAnalyzer::Result TypeAnalyzer::analyze_func_type(ASTNode *node) {
    ASTNode *params_node = node->get_child(FUNC_TYPE_PARAMS);
    ASTNode *return_type_node = node->get_child(FUNC_TYPE_RETURN_TYPE);

    std::vector<DataType *> param_types;
    for (ASTNode *param_node : params_node->get_children()) {
        Result param_result = analyze_param(param_node);
        if (param_result.result != SemaResult::OK) {
            return nullptr;
        }

        DataType *param_type = param_result.type;
        param_types.push_back(param_type);
    }

    Result return_result = analyze(return_type_node);
    if (return_result.result != SemaResult::OK) {
        return nullptr;
    }
    DataType *return_type = return_result.type;

    DataType *type = context.get_type_manager().new_data_type();
    type->set_to_function(FunctionType{param_types, return_type});
    node->as<Expr>()->set_data_type(type);
    return type;
}

TypeAnalyzer::Result TypeAnalyzer::analyze_static_array_type(ASTNode *node) {
    ASTNode *base_type_node = node->get_child(0);
    ASTNode *length_node = node->get_child(1);

    Result base_result = analyze(base_type_node);
    if (base_result.result != SemaResult::OK) {
        return base_result.result;
    }
    DataType *base_type = base_result.type;

    unsigned length = (unsigned)length_node->as<IntLiteral>()->get_value().to_u64();

    DataType *type = context.get_type_manager().new_data_type();
    type->set_to_static_array(StaticArrayType{base_type, length});
    node->as<Expr>()->set_data_type(type);
    return type;
}

TypeAnalyzer::Result TypeAnalyzer::analyze_tuple_type(ASTNode *node) {
    std::vector<DataType *> types;
    for (ASTNode *child : node->get_children()) {
        Result result = analyze(child);
        if (result.result != SemaResult::OK) {
            return result.result;
        }
        DataType *type = result.type;

        types.push_back(type);
    }

    DataType *type = context.get_type_manager().new_data_type();
    type->set_to_tuple({types});
    node->as<Expr>()->set_data_type(type);
    return type;
}

TypeAnalyzer::Result TypeAnalyzer::analyze_closure_type(ASTNode *node) {
    ASTNode *params_node = node->get_child(CLOSURE_TYPE_PARAMS);
    ASTNode *return_type_node = node->get_child(CLOSURE_TYPE_RETURN_TYPE);

    std::vector<DataType *> param_types;
    for (ASTNode *param_node : params_node->get_children()) {
        Result param_result = analyze_param(param_node);
        if (param_result.result != SemaResult::OK) {
            return param_result.result;
        }
        DataType *param_type = param_result.type;

        param_types.push_back(param_type);
    }

    Result return_result = analyze(return_type_node);
    if (return_result.result != SemaResult::OK) {
        return return_result.result;
    }
    DataType *return_type = return_result.type;

    DataType *type = context.get_type_manager().new_data_type();
    type->set_to_closure(FunctionType{param_types, return_type});
    node->as<Expr>()->set_data_type(type);
    return type;
}

TypeAnalyzer::Result TypeAnalyzer::analyze_generic_instance_type(ASTNode *node) {
    ASTNode *template_node = node->get_child(GENERIC_INSTANTIATION_TEMPLATE);
    ASTNode *args_node = node->get_child(GENERIC_INSTANTIATION_ARGS);

    ModulePath path = ASTUtils::get_path_from_node(template_node);
    SymbolTable *symbol_table = ASTUtils::find_symbol_table(node);
    GenericStruct *generic_struct = ASTUtils::resolve_generic_struct(path, symbol_table);

    if (!generic_struct) {
        return SemaResult::ERROR;
    }

    GenericStructInstance *instance = GenericsInstantiator(context).instantiate_struct(generic_struct, args_node);
    if (!instance) {
        return SemaResult::ERROR;
    }

    DataType *type = context.get_type_manager().new_data_type();
    type->set_to_structure(instance->entity);
    node->as<Expr>()->set_data_type(type);
    return type;
}

TypeAnalyzer::Result TypeAnalyzer::analyze_explicit_type(ASTNode *node) {
    ASTNode *type_node = node->get_child();

    Result result = TypeAnalyzer(context).analyze(type_node);
    node->as<Expr>()->set_data_type(result.type);
    return result;
}

DataType *TypeAnalyzer::from_symbol(SymbolRef symbol) {
    switch (symbol.get_kind()) {
        case SymbolKind::STRUCT: {
            DataType *type = context.get_type_manager().new_data_type();
            type->set_to_structure(symbol.get_struct());
            return type;
        }
        case SymbolKind::ENUM: {
            DataType *type = context.get_type_manager().new_data_type();
            type->set_to_enumeration(symbol.get_enum());
            return type;
        }
        case SymbolKind::UNION: {
            DataType *type = context.get_type_manager().new_data_type();
            type->set_to_union(symbol.get_union());
            return type;
        }
        case SymbolKind::UNION_CASE: {
            DataType *type = context.get_type_manager().new_data_type();
            type->set_to_union_case(symbol.get_union_case());
            return type;
        }
        case SymbolKind::PROTO: {
            DataType *type = context.get_type_manager().new_data_type();
            type->set_to_protocol(symbol.get_proto());
            return type;
        }
        case SymbolKind::TYPE_ALIAS: {
            context.get_sema().run_type_stage(symbol.get_type_alias()->get_node());
            return symbol.get_type_alias()->get_underlying_type();
        }
        default: return nullptr;
    }
}

void TypeAnalyzer::resolve_dot_operator_element(ASTNode *node, SymbolTable *&symbol_table) {
    if (node->get_type() == AST_IDENTIFIER) {
        const std::string &name = node->get_value();
        std::optional<SymbolRef> symbol = context.get_sema().resolve_symbol(symbol_table, name);

        if (!symbol) {
            context.register_error(node->get_range())
                .set_message(ReportText(ReportText::ERR_NO_VALUE).format(node).str());
            symbol_table = nullptr;
            return;
        }

        node->as<Identifier>()->set_symbol(*symbol);

        switch (symbol->get_kind()) {
            case SymbolKind::MODULE: symbol_table = ASTUtils::get_module_symbol_table(symbol->get_module()); break;
            case SymbolKind::STRUCT: symbol_table = symbol->get_struct()->get_symbol_table(); break;
            case SymbolKind::UNION: symbol_table = symbol->get_union()->get_symbol_table(); break;
            default: break;
        }
    } else if (node->get_type() == AST_DOT_OPERATOR) {
        resolve_dot_operator_element(node->get_child(0), symbol_table);
        if (!symbol_table) {
            symbol_table = nullptr;
            return;
        }

        resolve_dot_operator_element(node->get_child(1), symbol_table);
    } else {
        symbol_table = nullptr;
    }
}

DataType *TypeAnalyzer::instantiate_generic_std_type(
    ASTNode *node,
    const ModulePath &module_path,
    std::string name,
    GenericInstanceArgs args
) {
    ASTNode *module_node = context.get_module_manager().get_module_list().get_by_path(module_path);
    SymbolTable *module_symbol_table = ASTUtils::get_module_symbol_table(module_node);

    GenericStruct *generic_struct = module_symbol_table->get_generic_struct(name);
    if (!generic_struct) {
        return nullptr;
    }

    GenericStructInstance *instance = GenericsInstantiator(context).instantiate_struct(generic_struct, args);
    if (!instance) {
        return nullptr;
    }

    DataType *type = context.get_type_manager().new_data_type();
    type->set_to_structure(instance->entity);
    node->as<Expr>()->set_data_type(type);
    return type;
}

} // namespace lang

} // namespace banjo
