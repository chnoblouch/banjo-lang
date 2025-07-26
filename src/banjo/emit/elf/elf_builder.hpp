#ifndef BANJO_EMIT_ELF_BUILDER_H
#define BANJO_EMIT_ELF_BUILDER_H

#include "banjo/emit/binary_module.hpp"
#include "banjo/emit/elf/elf_format.hpp"
#include "banjo/target/target_description.hpp"

#include <string_view>
#include <vector>

namespace banjo {

class ELFBuilder {

private:
    target::TargetDescription target;

    ELFFile file;

    ELFSection text_section;
    ELFSection data_section;
    ELFSection shstrtab_section;
    ELFSection strtab_section;
    ELFSection symtab_section;
    ELFSection text_rela_section;
    ELFSection data_rela_section;
    std::optional<ELFSection> bnjatbl_section;
    std::optional<ELFSection> bnjatbl_rela_section;

    std::vector<std::uint32_t> elf_symbol_indices;

public:
    ELFBuilder(target::TargetDescription target);
    ELFFile build(BinModule module_);

private:
    void process_defs(const std::vector<BinSymbolDef> &defs);
    void process_def(const BinSymbolDef &def);
    void process_uses(const std::vector<BinSymbolUse> &uses);

    void process_x86_64_relocation(const BinSymbolUse &use, int &address_offset, std::uint32_t &type);
    void process_aarch64_relocation(const BinSymbolUse &use, int &address_offset, std::uint32_t &type);

    void add_section(ELFSection &section);
    std::uint32_t add_string(ELFSection &section, std::string_view string);
};

} // namespace banjo

#endif
