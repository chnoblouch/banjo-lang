#ifndef BANJO_EMIT_BINARY_BUILDER_SYMBOL_H
#define BANJO_EMIT_BINARY_BUILDER_SYMBOL_H

#include "banjo/emit/binary_module.hpp"

#include <cstdint>
#include <string>

namespace banjo {

struct SymbolDef {
    std::string name;
    BinSymbolKind kind;
    bool global;
    std::uint32_t slice_index;
    std::uint32_t local_offset;

    std::uint32_t bin_index;
    std::uint32_t bin_offset;
};

struct SymbolUse {
    std::uint32_t index;
    std::uint32_t local_offset;
    BinSymbolUseKind kind;
    std::int32_t addend;
    bool is_resolved = false;
};

} // namespace banjo

#endif
