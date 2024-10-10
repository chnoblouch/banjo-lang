#ifndef BANJO_LSP_INDEX_H
#define BANJO_LSP_INDEX_H

#include "banjo/reports/report.hpp"
#include "banjo/sir/sir.hpp"
#include "banjo/source/text_range.hpp"
#include "banjo/symbol/module_path.hpp"

#include <unordered_map>
#include <vector>

namespace banjo {

namespace lsp {

struct SymbolRef {
    lang::TextRange range;
    lang::sir::Symbol symbol;
    lang::sir::Module *def_mod;
    lang::TextRange def_range;
};

struct ModuleIndex {
    std::vector<SymbolRef> symbol_refs;
    std::vector<lang::Report> reports;
    std::vector<lang::ModulePath> dependents;
};

struct Index {
    std::unordered_map<lang::sir::Module *, ModuleIndex> mods;
};

} // namespace lsp

} // namespace banjo

#endif
