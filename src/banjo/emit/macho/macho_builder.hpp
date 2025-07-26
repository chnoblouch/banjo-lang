#ifndef BANJO_EMIT_MACHO_BUILDER
#define BANJO_EMIT_MACHO_BUILDER

#include "banjo/emit/binary_module.hpp"
#include "banjo/emit/macho/macho_format.hpp"

#include <cstdint>
#include <vector>

namespace banjo {

class MachOBuilder {

private:
    std::vector<MachOSymbol> symbols;
    std::vector<std::uint32_t> symbol_index_map;
    std::uint32_t num_local_symbols = 0;
    std::uint32_t num_external_symbols = 0;
    std::uint32_t num_undefined_symbols = 0;

    MachOSection text_section;
    MachOSection data_section;

public:
    MachOFile build(BinModule mod);

private:
    void process_defs(const std::vector<BinSymbolDef> &defs);
    void process_def(const BinSymbolDef &def, unsigned index);

    void process_uses(const std::vector<BinSymbolUse> &uses);
    void process_use(const BinSymbolUse &use);
    std::uint8_t to_relocation_type(BinSymbolUseKind use_kind);
};

} // namespace banjo

#endif
