#ifndef LSP_INDEX_H
#define LSP_INDEX_H

#include "banjo/symbol/symbol.hpp"
#include "banjo/symbol/symbol_ref.hpp"

#include <unordered_map>

namespace banjo {

namespace lsp {

struct IndexedSymbolRef;

struct IndexedSymbol {
    lang::Symbol symbol;
    lang::SymbolKind kind;
    std::vector<IndexedSymbolRef *> refs;
};

struct IndexedSymbolRef {
    lang::ASTNode *node;
    IndexedSymbolRef *symbol;
};

class Index {

private:
    std::unordered_map<lang::ASTNode, IndexedSymbol *> symbols;
    std::unordered_map<lang::ASTNode, IndexedSymbolRef *> refs;
};

} // namespace lsp

} // namespace banjo

#endif
