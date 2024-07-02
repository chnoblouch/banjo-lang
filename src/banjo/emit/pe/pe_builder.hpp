#ifndef PE_BUILDER_H
#define PE_BUILDER_H

#include "banjo/emit/binary_module.hpp"
#include "banjo/emit/pe/pe_format.hpp"

namespace banjo {

class PEBuilder {

private:
    static constexpr std::uint16_t TEXT_SECTION_INDEX = 0;
    static constexpr std::uint16_t DATA_SECTION_INDEX = 1;
    static constexpr std::uint16_t PDATA_SECTION_INDEX = 2;
    static constexpr std::uint16_t XDATA_SECTION_INDEX = 3;
    static constexpr std::uint16_t DRECTVE_SECTION_INDEX = 4;

    static constexpr std::uint16_t TEXT_SECTION_NUMBER = TEXT_SECTION_INDEX + 1;
    static constexpr std::uint16_t DATA_SECTION_NUMBER = DATA_SECTION_INDEX + 1;
    static constexpr std::uint16_t PDATA_SECTION_NUMBER = PDATA_SECTION_INDEX + 1;
    static constexpr std::uint16_t XDATA_SECTION_NUMBER = XDATA_SECTION_INDEX + 1;
    static constexpr std::uint16_t DRECTVE_SECTION_NUMBER = DRECTVE_SECTION_INDEX + 1;

    static constexpr std::uint32_t TEXT_SYMBOL_INDEX = 0;
    static constexpr std::uint32_t DATA_SYMBOL_INDEX = 1;
    static constexpr std::uint32_t EXTRA_SYMBOLS = 4;

    PEFile file;

public:
    PEFile build(BinModule module_);

private:
    void create_sections();
    void create_extra_symbols();

    void process_x86_64_symbol_def(const BinSymbolDef &def);
    void process_x86_64_symbol_use(const BinSymbolUse &use, BinModule &module_);

    void create_unwind_info(const std::vector<BinUnwindInfo> &unwind_info);
    void create_debug_info();
};

} // namespace banjo

#endif
