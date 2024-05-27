#include "decl_type_analyzer.hpp"

#include "ast/ast_child_indices.hpp"
#include "ast/ast_utils.hpp"
#include "ast/decl.hpp"
#include "ast/expr.hpp"
#include "sema/const_evaluator.hpp"
#include "sema/semantic_analyzer_context.hpp"
#include "sema/type_analyzer.hpp"
#include "symbol/global_variable.hpp"
#include "symbol/symbol_ref.hpp"
#include "symbol/union.hpp"

namespace lang {

DeclTypeAnalyzer::DeclTypeAnalyzer(SemanticAnalyzer &sema)
  : sema(sema),
    context(sema.get_context()),
    ast_context(context.get_ast_context()) {}

void DeclTypeAnalyzer::run(Symbol *symbol) {
    ASTNode *node = symbol->get_node();

    switch (node->get_type()) {
        case AST_FUNCTION_DEFINITION: analyze_func(node->as<ASTFunc>()); break;
        case AST_NATIVE_FUNCTION_DECLARATION: analyze_func(node->as<ASTFunc>()); break;
        case AST_CONSTANT: analyze_const(node->as<ASTConst>()); break;
        case AST_VAR: analyze_var(node->as<ASTVar>()); break;
        case AST_NATIVE_VAR: analyze_var(node->as<ASTVar>()); break;
        case AST_STRUCT_DEFINITION: analyze_struct(node->as<ASTStruct>()); break;
        case AST_ENUM_DEFINITION: analyze_enum(node->as<ASTEnum>()); break;
        case AST_UNION: analyze_union(node->as<ASTUnion>()); break;
        case AST_PROTO: analyze_proto(node->as<ASTProto>()); break;
        case AST_TYPE_ALIAS: analyze_type_alias(node->as<ASTTypeAlias>()); break;
        case AST_GENERIC_FUNCTION_DEFINITION: analyze_generic_func(node->as<ASTGenericFunc>()); break;
        case AST_GENERIC_STRUCT_DEFINITION: analyze_generic_struct(node->as<ASTGenericStruct>()); break;
        default: break;
    }

    node->set_sema_stage(SemaStage::TYPE);
}

bool DeclTypeAnalyzer::analyze_func(ASTFunc *node) {
    ASTNode *param_list = node->get_child(FUNC_PARAMS);
    ASTNode *type_node = node->get_child(FUNC_TYPE);

    auto [type, status] = analyze_func_type(param_list, type_node);

    Function *func = node->get_symbol();
    func->get_type() = type;
    func->set_param_names(collect_param_names(param_list));
    func->set_modifiers(Modifiers{.method = ASTUtils::is_func_method(node)});

    if (node->get_type() == AST_NATIVE_FUNCTION_DECLARATION) {
        node->get_symbol()->get_modifiers().native = true;
    }

    analyze_attrs(node, func);

    if (func->get_name() == "main" && sema.get_context().is_create_main_func()) {
        func->set_link_name("main");
        func->get_modifiers().exposed = true;
        func->set_used(true);
    }

    return status == SemaResult::OK;
}

bool DeclTypeAnalyzer::analyze_native_func(ASTFunc *node) {
    ASTNode *param_list = node->get_child(FUNC_PARAMS);
    ASTNode *type_node = node->get_child(FUNC_TYPE);

    auto [type, status] = analyze_func_type(param_list, type_node);
    if (status != SemaResult::OK) {
        return false;
    }

    Function *func = node->get_symbol();
    func->get_type() = type;
    func->set_param_names(collect_param_names(param_list));
    func->set_modifiers(Modifiers{.native = true});
    analyze_attrs(node, func);

    return true;
}

std::pair<FunctionType, SemaResult> DeclTypeAnalyzer::analyze_func_type(
    ASTNode *param_list,
    ASTNode *return_type_node
) {
    TypeAnalyzer::Result return_result = TypeAnalyzer(context).analyze(return_type_node);
    if (return_result.result != SemaResult::OK) {
        return {
            FunctionType{
                .param_types = {},
                .return_type = context.get_type_manager().get_primitive_type(PrimitiveType::VOID),
            },
            SemaResult::ERROR,
        };
    }
    DataType *return_type = return_result.type;

    std::optional<std::vector<DataType *>> param_types = analyze_params(param_list);
    if (!param_types) {
        return {
            FunctionType{
                .param_types = {},
                .return_type = return_type,
            },
            SemaResult::ERROR,
        };
    }

    return {
        FunctionType{
            .param_types = std::move(*param_types),
            .return_type = return_type,
        },
        SemaResult::OK,
    };
}

std::vector<std::string_view> DeclTypeAnalyzer::collect_param_names(ASTNode *param_list) {
    std::vector<std::string_view> param_names;
    param_names.resize(param_list->get_children().size());

    for (unsigned i = 0; i < param_list->get_children().size(); i++) {
        ASTNode *param_node = param_list->get_child(i);
        ASTNode *name_node = param_node->get_child(PARAM_NAME);

        if (name_node->get_type() == AST_SELF) {
            param_names[i] = {"self"};
        } else {
            param_names[i] = {name_node->get_value()};
        }
    }

    return param_names;
}

void DeclTypeAnalyzer::analyze_attrs(ASTNode *node, Function *func) {
    if (!node->get_attribute_list()) {
        return;
    }

    for (const Attribute &attr : node->get_attribute_list()->get_attributes()) {
        if (attr.get_name() == "exposed") func->get_modifiers().exposed = true;
        else if (attr.get_name() == "link_name") func->set_link_name(attr.get_value());
        else if (attr.get_name() == "dllexport") func->get_modifiers().dllexport = true;
        else if (attr.get_name() == "test") func->get_modifiers().test = true;
    }
}

std::optional<std::vector<DataType *>> DeclTypeAnalyzer::analyze_params(ASTNode *param_list) {
    DataTypeManager &type_manager = context.get_type_manager();
    std::vector<DataType *> types;

    for (ASTNode *parameter : param_list->get_children()) {
        ASTNode *name_node = parameter->get_child(PARAM_NAME);
        SymbolKind enclosing_symbol_kind = context.get_ast_context().enclosing_symbol.get_kind();

        if (name_node->get_type() != AST_SELF) {
            ASTNode *type_node = parameter->get_child(PARAM_TYPE);
            TypeAnalyzer::Result type_result = TypeAnalyzer(context).analyze(type_node);
            if (type_result.result != SemaResult::OK) {
                return {};
            }
            DataType *type = type_result.type;

            types.push_back(type);
        } else if (enclosing_symbol_kind == SymbolKind::STRUCT) {
            DataType *struct_type = type_manager.new_data_type();
            struct_type->set_to_structure(ast_context.enclosing_symbol.get_struct());

            DataType *param_type = type_manager.new_data_type();
            param_type->set_to_pointer(struct_type);
            types.push_back(param_type);
        } else if (enclosing_symbol_kind == SymbolKind::UNION) {
            DataType *union_type = type_manager.new_data_type();
            union_type->set_to_union(ast_context.enclosing_symbol.get_union());

            DataType *param_type = type_manager.new_data_type();
            param_type->set_to_pointer(union_type);
            types.push_back(param_type);
        } else if (enclosing_symbol_kind == SymbolKind::PROTO) {
            DataType *proto_type = type_manager.new_data_type();
            proto_type->set_to_protocol(ast_context.enclosing_symbol.get_proto());

            DataType *param_type = type_manager.new_data_type();
            param_type->set_to_pointer(proto_type);
            types.push_back(param_type);
        } else {
            context.register_error(name_node->get_range()).set_message(ReportText::ID::ERR_INVALID_SELF_PARAM);
            return {};
        }
    }

    return types;
}

bool DeclTypeAnalyzer::analyze_const(ASTConst *node) {
    ASTNode *name_node = node->get_child(CONST_NAME);
    ASTNode *type_node = node->get_child(CONST_TYPE);
    ASTNode *value_node = node->get_child(CONST_VALUE);

    TypeAnalyzer::Result type_result = TypeAnalyzer(context).analyze(type_node);
    if (type_result.result != SemaResult::OK) {
        return false;
    }
    DataType *type = type_result.type;

    Constant *const_ = node->get_symbol();
    const_->set_data_type(type);
    const_->set_value(value_node);
    name_node->as<Identifier>()->set_symbol(const_);

    return true;
}

bool DeclTypeAnalyzer::analyze_var(ASTVar *node) {
    ASTNode *type_node = node->get_child(VAR_TYPE);

    TypeAnalyzer::Result type_result = TypeAnalyzer(context).analyze(type_node);
    if (type_result.result != SemaResult::OK) {
        return false;
    }
    DataType *type = type_result.type;

    if (ast_context.enclosing_symbol.get_kind() == SymbolKind::STRUCT) {
        StructField *field = static_cast<StructField *>(node->get_symbol());
        field->set_type(type);

        if (node->get_attribute_list()) {
            for (const Attribute &attr : node->get_attribute_list()->get_attributes()) {
                if (attr.get_name() == "no_deinit") {
                    field->set_no_deinit(true);
                }
            }
        }
    } else {
        GlobalVariable *global = static_cast<GlobalVariable *>(node->get_symbol());
        global->set_data_type(type);
        analyze_attrs(node, global);
    }

    return true;
}

bool DeclTypeAnalyzer::analyze_native_var(ASTVar *node) {
    ASTNode *type_node = node->get_child(VAR_TYPE);

    TypeAnalyzer::Result type_result = TypeAnalyzer(context).analyze(type_node);
    if (type_result.result != SemaResult::OK) {
        return false;
    }
    DataType *type = type_result.type;

    GlobalVariable *global = static_cast<GlobalVariable *>(node->get_symbol());
    global->set_native(true);
    global->set_data_type(type);
    analyze_attrs(node, global);
    node->as<ASTVar>()->set_symbol(global);

    return true;
}

void DeclTypeAnalyzer::analyze_attrs(ASTNode *node, GlobalVariable *global) {
    if (!node->get_attribute_list()) {
        return;
    }

    for (const Attribute &attr : node->get_attribute_list()->get_attributes()) {
        if (attr.get_name() == "exposed") {
            global->set_exposed(true);
        } else if (attr.get_name() == "link_name") {
            global->set_link_name(attr.get_value());
        }
    }
}

bool DeclTypeAnalyzer::analyze_struct(ASTStruct *node) {
    ASTNode *block = node->get_child(STRUCT_BLOCK);

    context.push_ast_context().enclosing_symbol = node->get_symbol();
    for (ASTNode *child : block->get_children()) {
        sema.run_type_stage(child);
    }
    context.pop_ast_context();

    return true;
}

bool DeclTypeAnalyzer::analyze_enum(ASTEnum *node) {
    ASTNode *variants_node = node->get_child(ENUM_VARIANTS);

    Enumeration *enum_ = node->get_symbol();
    LargeInt value = -1;

    for (ASTNode *child : variants_node->get_children()) {
        ASTEnumVariant *variant_node = child->as<ASTEnumVariant>();

        ASTNode *variant_name_node = variant_node->get_child(ENUM_VARIANT_NAME);
        std::string name = variant_name_node->get_value();

        if (variant_node->get_children().size() > 1) {
            ASTNode *variant_value_node = variant_node->get_child(ENUM_VARIANT_VALUE);
            value = ConstEvaluator().eval_int(variant_value_node);
        } else {
            value = value + 1;
        }

        variant_node->set_symbol(EnumVariant(child, name, value, enum_));
        variant_name_node->as<Identifier>()->set_symbol(variant_node->get_symbol());
        enum_->get_symbol_table()->add_enum_variant(variant_node->get_symbol());
    }

    return true;
}

bool DeclTypeAnalyzer::analyze_union(ASTUnion *node) {
    ASTNode *block_node = node->get_child(UNION_BLOCK);

    Union *union_ = node->get_symbol();
    unsigned case_index = 0;

    for (ASTNode *child : block_node->get_children()) {
        if (child->get_type() != AST_UNION_CASE) {
            continue;
        }

        ASTNode *fields_node = child->get_child(UNION_CASE_FIELDS);
        UnionCase *case_ = union_->get_case(case_index);

        for (ASTNode *field_node : fields_node->get_children()) {
            Identifier *name_node = field_node->get_child(UNION_CASE_FIELD_NAME)->as<Identifier>();
            ASTNode *type_node = field_node->get_child(UNION_CASE_FIELD_TYPE);

            TypeAnalyzer::Result result = TypeAnalyzer(context).analyze(type_node);
            if (result.result != SemaResult::OK) {
                return false;
            }

            const std::string &name = name_node->get_value();
            UnionCaseField *field = new UnionCaseField(field_node, name, result.type);

            case_->add_field(field);
            name_node->set_symbol(field);
        }

        case_index += 1;
    }

    context.push_ast_context().enclosing_symbol = node->get_symbol();
    for (ASTNode *child : block_node->get_children()) {
        sema.run_type_stage(child);
    }
    context.pop_ast_context();

    return true;
}

bool DeclTypeAnalyzer::analyze_proto(ASTProto *node) {
    ASTNode *block = node->get_child(PROTO_BLOCK);

    context.push_ast_context().enclosing_symbol = node->get_symbol();
    bool ok = true;

    for (ASTNode *child : block->get_children()) {
        if (child->get_type() != AST_FUNC_DECL) {
            continue;
        }

        ASTNode *name_node = child->get_child(FUNC_NAME);
        ASTNode *param_list = child->get_child(FUNC_PARAMS);
        ASTNode *type_node = child->get_child(FUNC_TYPE);

        auto [type, status] = analyze_func_type(param_list, type_node);
        if (status != SemaResult::OK) {
            ok = false;
        }

        node->get_symbol()->add_func_signature(name_node->get_value(), type);
    }

    context.pop_ast_context();
    return ok;
}

bool DeclTypeAnalyzer::analyze_type_alias(ASTTypeAlias *node) {
    ASTNode *underlying_type_node = node->get_child(TYPE_ALIAS_UNDERLYING_TYPE);

    TypeAnalyzer::Result result = TypeAnalyzer(context).analyze(underlying_type_node);
    if (result.result != SemaResult::OK) {
        return false;
    }

    node->get_symbol()->set_underlying_type(result.type);
    return true;
}

bool DeclTypeAnalyzer::analyze_generic_func(ASTGenericFunc *node) {
    ASTNode *generic_params_node = node->get_child(GENERIC_FUNC_GENERIC_PARAMS);

    GenericFunc *generic_func = node->get_symbol();
    collect_generic_params(generic_params_node, generic_func->get_generic_params());
    return true;
}

bool DeclTypeAnalyzer::analyze_generic_struct(ASTGenericStruct *node) {
    ASTNode *generic_params_node = node->get_child(GENERIC_STRUCT_GENERIC_PARAMS);

    GenericStruct *generic_struct = node->get_symbol();
    collect_generic_params(generic_params_node, generic_struct->get_generic_params());
    return true;
}

void DeclTypeAnalyzer::collect_generic_params(ASTNode *generic_param_list, std::vector<GenericParam> &generic_params) {
    for (ASTNode *param_node : generic_param_list->get_children()) {
        ASTNode *name_node = param_node->get_child(GENERIC_PARAM_NAME);

        GenericParam param{
            .name = name_node->get_value(),
            .is_param_sequence = false,
        };

        if (param_node->get_children().size() > 1) {
            ASTNode *type_node = param_node->get_child(GENERIC_PARAM_TYPE);
            param.is_param_sequence = type_node->get_type() == AST_PARAM_SEQUENCE_TYPE;
        }

        generic_params.push_back(param);
    }
}

} // namespace lang
