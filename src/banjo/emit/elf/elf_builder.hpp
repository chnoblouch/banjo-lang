#ifndef ELF_BUILDER_H
#define ELF_BUILDER_H

#include "emit/binary_module.hpp"
#include "emit/elf/elf_format.hpp"

namespace banjo {

class ELFBuilder {

private:
    ELFFile file;

    ELFSection text_section;
    ELFSection data_section;
    ELFSection shstrtab_section;
    ELFSection strtab_section;
    ELFSection symtab_section;
    ELFSection text_rela_section;

    std::vector<std::uint32_t> elf_symbol_indices;

public:
    ELFFile build(BinModule module_);

private:
    void process_defs(const std::vector<BinSymbolDef> &defs);
    void process_def(const BinSymbolDef &def);

    void add_section(ELFSection &section);
    std::uint32_t add_string(ELFSection &section, std::string string);
};

} // namespace banjo

#endif
