#ifndef SEMA_EXTRA_ANALYSIS_H
#define SEMA_EXTRA_ANALYSIS_H

#include "banjo/sir/sir.hpp"
#include "banjo/source/text_range.hpp"

#include <unordered_map>
#include <vector>

namespace banjo {

namespace lang {

namespace sema {

struct ExtraAnalysis {
    struct SymbolDef {
        sir::Symbol symbol;
        TextRange ident_range;
    };

    struct SymbolUse {
        TextRange range;
        sir::Symbol symbol;
    };

    struct ModuleAnalysis {
        std::vector<SymbolDef> symbol_defs;
        std::vector<SymbolUse> symbol_uses;
    };

    std::unordered_map<sir::Module *, ModuleAnalysis> mods;
};

} // namespace sema

} // namespace lang

} // namespace banjo

#endif
