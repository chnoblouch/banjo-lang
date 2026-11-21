#ifndef BANJO_SEMA_CLOSURE_CONTEXT_H
#define BANJO_SEMA_CLOSURE_CONTEXT_H

#include "banjo/sir/sir.hpp"

#include <optional>
#include <vector>

namespace banjo::sema {

struct ClosureContext {
    std::vector<sir::Symbol> captured_vars;
    sir::TupleExpr *data_type;
    sir::SymbolTable *parent_symbol_table;

    std::optional<unsigned> try_capture(sir::Symbol symbol);
};

} // namespace banjo::sema

#endif
