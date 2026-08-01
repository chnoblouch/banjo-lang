#ifndef BANJO_LSP_INDEX_H
#define BANJO_LSP_INDEX_H

#include "banjo/reports/report.hpp"
#include "banjo/sir/sir.hpp"
#include "banjo/source/module_path.hpp"
#include "banjo/source/text_range.hpp"

#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace banjo::lsp {

struct SymbolKey {
    sir::Module *mod;
    unsigned index;
};

struct SymbolRef {
    TextRange range;
    sir::Symbol symbol;
    sir::Module *def_mod;
    TextRange def_range;
    // std::vector<SymbolKey> uses;
};

struct ModuleIndex {
    std::vector<SymbolRef> symbol_refs;
    std::vector<Report> reports;
    std::unordered_set<ModulePath> dependents;
};

struct Index {
    std::unordered_map<sir::Module *, ModuleIndex> mods;

    SymbolRef &get_symbol(SymbolKey key) { return mods.at(key.mod).symbol_refs[key.index]; }
};

} // namespace banjo::lsp

#endif
