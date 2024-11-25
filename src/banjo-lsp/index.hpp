#ifndef BANJO_LSP_INDEX_H
#define BANJO_LSP_INDEX_H

#include "banjo/reports/report.hpp"
#include "banjo/sir/sir.hpp"
#include "banjo/source/text_range.hpp"
#include "banjo/source/module_path.hpp"

#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace banjo {

namespace lsp {

struct SymbolKey {
    lang::sir::Module *mod;
    unsigned index;
};

struct SymbolRef {
    lang::TextRange range;
    lang::sir::Symbol symbol;
    lang::sir::Module *def_mod;
    lang::TextRange def_range;
    // std::vector<SymbolKey> uses;
};

struct ModuleIndex {
    std::vector<SymbolRef> symbol_refs;
    std::vector<lang::Report> reports;
    std::unordered_set<lang::ModulePath> dependents;
};

struct Index {
    std::unordered_map<lang::sir::Module *, ModuleIndex> mods;

    SymbolRef &get_symbol(SymbolKey key) { return mods.at(key.mod).symbol_refs[key.index]; }
};

} // namespace lsp

} // namespace banjo

#endif
