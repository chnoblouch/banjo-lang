#ifndef BANJO_EMIT_PE_BUILDER_H
#define BANJO_EMIT_PE_BUILDER_H

#include "banjo/emit/binary_module.hpp"
#include "banjo/emit/pe/pe_format.hpp"

namespace banjo {

class PEBuilder {

private:
    static constexpr std::uint16_t TEXT_SECTION_INDEX = 0;
    static constexpr std::uint16_t DATA_SECTION_INDEX = 1;
    static constexpr std::uint16_t PDATA_SECTION_INDEX = 2;
    static constexpr std::uint16_t XDATA_SECTION_INDEX = 3;

    PEFile file;

    unsigned drectve_section_index;
    unsigned bnjatbl_section_index;

    unsigned num_section_symbols;

public:
    PEFile build(BinModule module_);

private:
    void create_sections(BinModule &module_);
    void create_section_symbols();

    void process_x86_64_symbol_def(const BinSymbolDef &def);
    void process_x86_64_symbol_use(const BinSymbolUse &use, BinModule &module_);

    void create_unwind_info(const std::vector<BinUnwindInfo> &unwind_info);
    void create_debug_info();
    void move_section_data(BinModule &module_);

    std::int16_t get_section_number(unsigned section_index);
    std::uint32_t get_section_symbol_index(unsigned section_index);
};

} // namespace banjo

#endif
