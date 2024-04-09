#include "completion_handler.hpp"

#include "ast/ast_child_indices.hpp"
#include "ast/ast_module.hpp"
#include "ast/ast_node.hpp"
#include "ast/ast_utils.hpp"
#include "ast/decl.hpp"
#include "ast/expr.hpp"
#include "ast_navigation.hpp"
#include "json.hpp"
#include "reports/report_utils.hpp"
#include "symbol/data_type.hpp"
#include "symbol/structure.hpp"
#include "symbol/symbol_ref.hpp"
#include "symbol/symbol_table.hpp"
#include "uri.hpp"

#include <set>
#include <string_view>

namespace lsp {

CompletionHandler::CompletionHandler(SourceManager &source_manager) : source_manager(source_manager) {}

CompletionHandler::~CompletionHandler() {}

JSONValue CompletionHandler::handle(const JSONObject &params, Connection &connection) {
    std::string uri = params.get_object("textDocument").get_string("uri");
    std::filesystem::path path = URI::decode_to_path(uri);
    const SourceFile &file = source_manager.get_file(path);

    const JSONObject &lsp_position = params.get_object("position");
    int line = lsp_position.get_number("line");
    int column = lsp_position.get_number("character");
    lang::TextPosition position = ASTNavigation::pos_from_lsp(file.source, line, column);

    CompletionContext context = source_manager.parse_for_completion(path, position);
    if (!context.node) {
        return JSONArray();
    }

    source_manager.analyze_isolated_module(context.module_node);
    return build_items(context);
}

JSONArray CompletionHandler::build_items(const CompletionContext &context) {
    JSONArray array;

    if (!context.node->get_parent()) {
        return array;
    }

    if (context.node->get_parent()->get_type() == lang::AST_DOT_OPERATOR) {
        return complete_after_dot(context.node);
    } else if (context.node->get_parent()->get_type() == lang::AST_USE_TREE_LIST) {
        return complete_inside_use_tree_list(context.node);
    } else if (context.node->get_parent()->get_type() == lang::AST_STRUCT_FIELD_VALUE_LIST) {
        lang::ASTNode *struct_literal_node = context.node->get_parent()->get_parent();
        lang::DataType *type = struct_literal_node->as<lang::Expr>()->get_data_type();
        if (!type) {
            return array;
        }

        std::set<std::string_view> assigned_fields;
        for (lang::ASTNode *field_value : context.node->get_parent()->get_children()) {
            if (field_value->get_type() != lang::AST_STRUCT_FIELD_VALUE) {
                continue;
            }

            lang::ASTNode *field_name_node = field_value->get_child(lang::STRUCT_FIELD_VALUE_NAME);
            assigned_fields.insert(field_name_node->get_value());
        }

        lang::Structure *struct_ = type->get_structure();

        for (lang::StructField *field : struct_->get_fields()) {
            if (assigned_fields.contains(field->get_name())) {
                continue;
            }

            array.add(JSONObject{
                {"label", field->get_name()},
                {"kind", LSPCompletionItemKind::FIELD},
                {"insertText", field->get_name() + ": "},
            });
        }
    } else {
        lang::SymbolTable *symbol_table = lang::ASTUtils::find_symbol_table(context.node);

        while (symbol_table) {
            complete_symbol_table_members(context.node, symbol_table, array);
            symbol_table = symbol_table->get_parent();
        }

        if (context.node->get_parent()->get_type() == lang::AST_BLOCK) {
            complete_inside_block(context.node, array);
        }
    }

    return array;
}

JSONArray CompletionHandler::complete_after_dot(lang::ASTNode *node) {
    lang::ASTNode *lhs = node->get_parent()->get_child(0);

    lang::ASTNode *previous_node;
    if (lhs->get_type() == lang::AST_IDENTIFIER || lhs->get_type() == lang::AST_SELF ||
        lhs->get_type() == lang::AST_FUNCTION_CALL) {
        previous_node = lhs;
    } else if (lhs->get_type() == lang::AST_DOT_OPERATOR) {
        previous_node = lhs->get_child(1);
    } else {
        return JSONArray();
    }

    lang::DataType *type = previous_node->as<lang::Expr>()->get_data_type();
    if (type) {
        if (type->get_kind() == lang::DataType::Kind::STRUCT) {
            return complete_struct_instance_members(type->get_structure());
        } else if (type->get_kind() == lang::DataType::Kind::POINTER) {
            if (type->get_base_data_type()->get_kind() == lang::DataType::Kind::STRUCT) {
                return complete_struct_instance_members(type->get_base_data_type()->get_structure());
            }
        }
    }

    if (previous_node->get_type() == lang::AST_IDENTIFIER) {
        lang::Identifier *identifier = previous_node->as<lang::Identifier>();

        if (identifier->get_symbol()) {
            lang::SymbolRef symbol = *identifier->get_symbol();

            switch (symbol.get_kind()) {
                case lang::SymbolKind::MODULE: return complete_module_members(previous_node);
                case lang::SymbolKind::STRUCT: return complete_struct_members(previous_node);
                case lang::SymbolKind::ENUM: return complete_enum_members(previous_node);
                default: break;
            }
        }
    }

    return JSONArray();
}

JSONArray CompletionHandler::complete_inside_use_tree_list(lang::ASTNode *node) {
    lang::ASTNode *use_tree = node->get_parent();
    lang::ASTNode *dot_operator = use_tree->get_parent();
    lang::ASTNode *lhs = dot_operator->get_child(0);

    lang::ASTNode *previous_node;
    if (lhs->get_type() == lang::AST_IDENTIFIER) {
        previous_node = lhs;
    } else if (lhs->get_type() == lang::AST_DOT_OPERATOR) {
        previous_node = lhs->get_child(1);
    } else {
        return JSONArray();
    }

    return complete_module_members(previous_node);
}

JSONArray CompletionHandler::complete_module_members(lang::ASTNode *node) {
    lang::ASTModule *module_ = node->as<lang::Identifier>()->get_symbol()->get_module();
    lang::ModulePath module_path = module_->get_path();
    lang::SymbolTable *symbol_table = lang::ASTUtils::get_module_symbol_table(module_);

    JSONArray array;
    complete_symbol_table_members(node, symbol_table, array);
    return array;
}

JSONArray CompletionHandler::complete_struct_members(lang::ASTNode *node) {
    lang::Structure *struct_ = node->as<lang::Identifier>()->get_symbol()->get_struct();
    lang::SymbolTable *symbol_table = struct_->get_symbol_table();

    JSONArray array;
    complete_symbol_table_members(node, symbol_table, array);
    return array;
}

JSONArray CompletionHandler::complete_enum_members(lang::ASTNode *node) {
    lang::Enumeration *enum_ = node->as<lang::Identifier>()->get_symbol()->get_enum();
    lang::SymbolTable *symbol_table = enum_->get_symbol_table();

    JSONArray array;
    complete_symbol_table_members(node, symbol_table, array);
    return array;
}

void CompletionHandler::complete_symbol_table_members(
    lang::ASTNode *node,
    lang::SymbolTable *symbol_table,
    JSONArray &array
) {
    CompletionKind kind = get_completion_kind(node);

    for (auto [name, symbol] : symbol_table->get_symbols()) {
        std::optional<lang::SymbolRef> resolved_symbol = symbol.resolve();
        if (!resolved_symbol || resolved_symbol->get_kind() == lang::SymbolKind::FIELD) {
            continue;
        }

        bool link = resolved_symbol->is_link();
        bool sub_module = resolved_symbol->is_sub_module();

        switch (kind) {
            case CompletionKind::EXPR: break;

            case CompletionKind::AFTER_DOT:
            case CompletionKind::BEFORE_USE_TREE:
            case CompletionKind::INSIDE_USE_TREE:
                if (link && !sub_module) continue;
                break;
        }

        array.add(build_symbol_item(kind, name, *resolved_symbol));
    }

    if (kind == CompletionKind::BEFORE_USE_TREE) {
        for (const lang::ModulePath &path : source_manager.get_module_manager().enumerate_root_paths()) {
            array.add(JSONObject{
                {"label", path.get(0)},
                {"kind", LSPCompletionItemKind::MODULE},
            });
        }
    }
}

CompletionHandler::CompletionKind CompletionHandler::get_completion_kind(lang::ASTNode *node) {
    lang::ASTNode *parent = node->get_parent();
    if (!parent) {
        return CompletionKind::EXPR;
    }

    if (parent->get_type() == lang::AST_USE && node->get_type() == lang::AST_IDENTIFIER) {
        return CompletionKind::BEFORE_USE_TREE;
    }

    CompletionKind kind = CompletionKind::EXPR;

    if (parent->get_type() == lang::AST_DOT_OPERATOR) {
        kind = CompletionKind::AFTER_DOT;
    }

    while (parent) {
        if (parent->get_type() == lang::AST_USE) {
            kind = CompletionKind::INSIDE_USE_TREE;
            break;
        }

        parent = parent->get_parent();
    }

    return kind;
}

JSONObject CompletionHandler::build_symbol_item(
    CompletionKind kind,
    const std::string &name,
    const lang::SymbolRef &symbol
) {
    if (kind == CompletionKind::EXPR || kind == CompletionKind::AFTER_DOT) {
        if (symbol.get_kind() == lang::SymbolKind::FUNCTION) {
            return build_func_item(symbol.get_func());
        } else if (symbol.get_kind() == lang::SymbolKind::GENERIC_FUNC) {
            return build_generic_func_item(symbol.get_generic_func());
        } else if (symbol.get_kind() == lang::SymbolKind::GROUP) {
            return build_overload_group_item(name);
        }
    }

    return JSONObject{{"label", name}, {"kind", get_lsp_kind(symbol)}};
}

CompletionHandler::LSPCompletionItemKind CompletionHandler::get_lsp_kind(const lang::SymbolRef &symbol) {
    switch (symbol.get_kind()) {
        case lang::SymbolKind::MODULE: return LSPCompletionItemKind::MODULE;
        case lang::SymbolKind::FUNCTION:
            return symbol.get_func()->is_method() ? LSPCompletionItemKind::METHOD : LSPCompletionItemKind::FUNCTION;
        case lang::SymbolKind::GLOBAL: return LSPCompletionItemKind::VARIABLE;
        case lang::SymbolKind::CONST: return LSPCompletionItemKind::CONSTANT;
        case lang::SymbolKind::STRUCT: return LSPCompletionItemKind::STRUCT;
        case lang::SymbolKind::ENUM: return LSPCompletionItemKind::ENUM;
        case lang::SymbolKind::ENUM_VARIANT: return LSPCompletionItemKind::ENUM_MEMBER;
        case lang::SymbolKind::GENERIC_FUNC: return LSPCompletionItemKind::FUNCTION;
        case lang::SymbolKind::GENERIC_STRUCT: return LSPCompletionItemKind::STRUCT;
        default: return LSPCompletionItemKind::FUNCTION;
    }
}

JSONArray CompletionHandler::complete_struct_instance_members(lang::Structure *struct_) {
    JSONArray array;

    for (lang::StructField *member : struct_->get_fields()) {
        array.add(JSONObject{{"label", member->get_name()}, {"kind", LSPCompletionItemKind::FIELD}});
    }

    for (lang::Function *func : struct_->get_method_table().get_functions()) {
        array.add(build_func_item(func));
    }

    return array;
}

void CompletionHandler::complete_inside_block(lang::ASTNode *node, JSONArray &array) {
    array.add(JSONObject{
        {"label", "var"},
        {"kind", LSPCompletionItemKind::KEYWORD},
        {"insertText", "var "},
    });

    array.add(JSONObject{
        {"label", "if"},
        {"kind", LSPCompletionItemKind::KEYWORD},
        {"insertText", "if ${1:condition} {\n\t$0\n}"},
        {"insertTextFormat", 2},
    });

    array.add(JSONObject{
        {"label", "while"},
        {"kind", LSPCompletionItemKind::KEYWORD},
        {"insertText", "while ${1:condition} {\n\t$0\n}"},
        {"insertTextFormat", 2},
    });

    array.add(JSONObject{
        {"label", "for"},
        {"kind", LSPCompletionItemKind::KEYWORD},
        {"insertText", "for ${1:n} in ${2:iterator} {\n\t$0\n}"},
        {"insertTextFormat", 2},
    });

    array.add(JSONObject{
        {"label", "return"},
        {"kind", LSPCompletionItemKind::KEYWORD},
        {"insertText", "return $0;"},
        {"insertTextFormat", 2},
    });
}

JSONObject CompletionHandler::build_func_item(lang::Function *func) {
    std::vector<lang::DataType *> &param_types = func->get_type().param_types;

    std::string detail = "(";
    for (int i = 0; i < param_types.size(); i++) {
        if (!(func->is_method() && i == 0)) {
            detail += lang::ReportUtils::type_to_string(param_types[i]);
        } else {
            detail += "self";
        }

        if (i != param_types.size() - 1) {
            detail += ", ";
        }
    }
    detail += ")";

    lang::DataType *return_type = func->get_type().return_type;
    if (return_type && !return_type->is_primitive_of_type(lang::PrimitiveType::VOID)) {
        detail += " -> " + lang::ReportUtils::type_to_string(return_type);
    }

    std::string insert_text = func->get_name() + "(";
    for (unsigned i = func->is_method() ? 1 : 0; i < func->get_param_names().size(); i++) {
        std::string_view param_name = func->get_param_names()[i];
        if (param_name.empty()) {
            param_name = "_";
        }

        insert_text += "${" + std::to_string(i + 1) + ":" + std::string{param_name} + "}";
        if (i != func->get_param_names().size() - 1) {
            insert_text += ", ";
        }
    }
    insert_text += ")";

    return JSONObject{
        {"label", func->get_name()},
        {"labelDetails", JSONObject{{"detail", detail}}},
        {"kind", func->is_method() ? LSPCompletionItemKind::METHOD : LSPCompletionItemKind::FUNCTION},
        {"insertText", insert_text},
        {"insertTextFormat", 2},
    };
}

JSONObject CompletionHandler::build_overload_group_item(const std::string &name) {
    return JSONObject{
        {"label", name},
        {"labelDetails", JSONObject{{"detail", "(...)"}}},
        {"kind", LSPCompletionItemKind::FUNCTION},
        {"insertText", name + "(${1:})"},
        {"insertTextFormat", 2},
    };
}

JSONObject CompletionHandler::build_generic_func_item(lang::GenericFunc *generic_func) {
    return JSONObject{
        {"label", generic_func->get_name()},
        {"labelDetails", JSONObject{{"detail", "(...)"}}},
        {"kind", LSPCompletionItemKind::FUNCTION},
        {"insertText", generic_func->get_name() + "(${1:})"},
        {"insertTextFormat", 2},
    };
}

} // namespace lsp
