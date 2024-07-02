#include "symbol_ref_info.hpp"

#include "ast/ast_child_indices.hpp"
#include "ast/ast_node.hpp"
#include "ast/ast_utils.hpp"
#include "ast/expr.hpp"
#include "symbol/enumeration.hpp"
#include "symbol/protocol.hpp"
#include "symbol/symbol.hpp"
#include "symbol/symbol_ref.hpp"
#include "utils/macros.hpp"

namespace banjo {

namespace lsp {

std::optional<SymbolRefInfo> SymbolRefInfo::look_up(lang::Identifier *identifier) {
    if (!identifier->get_symbol()) {
        return {};
    }

    SymbolRefInfo info;

    std::optional<lang::SymbolRef> symbol_ref = identifier->get_symbol()->resolve();
    if (!symbol_ref) {
        return {};
    }

    info.kind = symbol_ref->get_kind();

    switch (info.kind) {
        case lang::SymbolKind::NONE: return {};
        case lang::SymbolKind::MODULE: return {};
        case lang::SymbolKind::FUNCTION: info.init_from_symbol(symbol_ref->get_func(), lang::FUNC_NAME); break;
        case lang::SymbolKind::LOCAL: info.init_from_local(*symbol_ref); break;
        case lang::SymbolKind::PARAM: info.init_from_symbol(symbol_ref->get_param(), lang::PARAM_NAME); break;
        case lang::SymbolKind::GLOBAL: info.init_from_symbol(symbol_ref->get_global(), lang::VAR_NAME); break;
        case lang::SymbolKind::CONST: info.init_from_symbol(symbol_ref->get_const(), lang::CONST_NAME); break;
        case lang::SymbolKind::STRUCT: info.init_from_symbol(symbol_ref->get_struct(), lang::STRUCT_NAME); break;
        case lang::SymbolKind::FIELD: info.init_from_symbol(symbol_ref->get_field(), lang::VAR_NAME); break;
        case lang::SymbolKind::ENUM: info.init_from_symbol(symbol_ref->get_enum(), lang::ENUM_NAME); break;
        case lang::SymbolKind::ENUM_VARIANT:
            info.init_from_symbol(symbol_ref->get_enum_variant(), lang::ENUM_VARIANT_NAME);
            break;
        case lang::SymbolKind::UNION: info.init_from_symbol(symbol_ref->get_union(), lang::UNION_NAME); break;
        case lang::SymbolKind::UNION_CASE:
            info.init_from_symbol(symbol_ref->get_union_case(), lang::UNION_CASE_NAME);
            break;
        case lang::SymbolKind::UNION_CASE_FIELD:
            info.init_from_symbol(symbol_ref->get_union_case_field(), lang::UNION_CASE_FIELD_NAME);
            break;
        case lang::SymbolKind::PROTO: info.init_from_symbol(symbol_ref->get_proto(), lang::PROTO_NAME); break;
        case lang::SymbolKind::TYPE_ALIAS:
            info.init_from_symbol(symbol_ref->get_type_alias(), lang::TYPE_ALIAS_NAME);
            break;
        case lang::SymbolKind::GENERIC_FUNC:
            info.init_from_symbol(symbol_ref->get_generic_func(), lang::GENERIC_FUNC_NAME);
            break;
        case lang::SymbolKind::GENERIC_STRUCT:
            info.init_from_symbol(symbol_ref->get_generic_struct(), lang::GENERIC_STRUCT_NAME);
            break;
        case lang::SymbolKind::USE: return {};
        case lang::SymbolKind::GROUP: return {};
    }

    info.is_definition = info.definition_ident == identifier;
    return info;
}

void SymbolRefInfo::init_from_symbol(lang::Symbol *symbol, unsigned ident_index_in_def) {
    this->symbol = symbol;
    definition_ident = symbol->get_node()->get_child(ident_index_in_def)->as<lang::Identifier>();
}

void SymbolRefInfo::init_from_local(const lang::SymbolRef &symbol_ref) {
    symbol = symbol_ref.get_local();

    lang::ASTNode *definition = symbol->get_node();

    if (definition->get_type() == lang::AST_VAR) {
        definition_ident = definition->get_child(lang::VAR_NAME)->as<lang::Identifier>();
    } else if (definition->get_type() == lang::AST_IMPLICIT_TYPE_VAR) {
        definition_ident = definition->get_child(lang::TYPE_INFERRED_VAR_NAME)->as<lang::Identifier>();
    } else {
        ASSERT_UNREACHABLE;
    }
}

lang::ASTModule *SymbolRefInfo::find_definition_mod() {
    return lang::ASTUtils::find_module(definition_ident);
}

} // namespace lsp

} // namespace banjo
