#ifndef SYMBOL_REF_INFO_H
#define SYMBOL_REF_INFO_H

#include "banjo/ast/ast_module.hpp"
#include "banjo/ast/expr.hpp"
#include "banjo/symbol/symbol.hpp"
#include "banjo/symbol/symbol_ref.hpp"

#include <optional>

namespace banjo {

namespace lsp {

// TODO: replace this with an index
class SymbolRefInfo {

public:
    bool is_valid;
    lang::Symbol *symbol;
    lang::SymbolKind kind;
    bool is_definition;
    lang::Identifier *definition_ident;

    static std::optional<SymbolRefInfo> look_up(lang::Identifier *identifier);
    lang::ASTModule *find_definition_mod();

private:
    void init_from_symbol(lang::Symbol *symbol, unsigned ident_index_in_def);
    void init_from_local(const lang::SymbolRef &symbol_ref);
};

} // namespace lsp

} // namespace banjo

#endif
