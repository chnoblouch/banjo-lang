#include "elf_builder.hpp"

#include "banjo/emit/elf/elf_format.hpp"
#include "banjo/utils/macros.hpp"

namespace banjo {

ELFFile ELFBuilder::build(BinModule module_) {
    file.machine = ELFMachine::X86_64;

    /*
    TODO: Where do these alignment values come from?
    We currently just use the ones that Clang and NASM emit...
    */

    shstrtab_section = ELFSection{
        .type = ELFSectionType::STRTAB,
        .flags = 0,
        .alignment = 1,
        .data = ELFSection::BinaryData{'\0'}, // The first byte in a string table is defined to be null.
    };

    text_section = ELFSection{
        .name_offset = add_string(shstrtab_section, ".text"),
        .type = ELFSectionType::PROGBITS,
        .flags = ELFSectionFlags::ALLOC | ELFSectionFlags::EXECINSTR,
        .alignment = 16,
        .data = module_.text.move_data(),
    };

    data_section = ELFSection{
        .name_offset = add_string(shstrtab_section, ".data"),
        .type = ELFSectionType::PROGBITS,
        .flags = ELFSectionFlags::ALLOC | ELFSectionFlags::WRITE,
        .alignment = 16, // FIXME: This is usually 4, but required to be 16 for aligning SSE constants.
        .data = module_.data.move_data(),
    };

    shstrtab_section.name_offset = add_string(shstrtab_section, ".shstrtab");

    strtab_section = ELFSection{
        .name_offset = add_string(shstrtab_section, ".strtab"),
        .type = ELFSectionType::STRTAB,
        .alignment = 1,
        .entry_size = 0,
        .data = ELFSection::BinaryData{'\0'}, // The first byte in a string table is defined to be null.
    };

    symtab_section = ELFSection{
        .name_offset = add_string(shstrtab_section, ".symtab"),
        .type = ELFSectionType::SYMTAB,
        .link = 4,
        .alignment = 8,
        .entry_size = 24,
        .data =
            ELFSection::SymbolList{
                ELFSymbol{
                    .name_offset = 0,
                    .binding = 0,
                    .type = ELFSymbolType::NOTYPE,
                    .section_index = 0,
                    .value = 0,
                    .size = 0,
                },
                ELFSymbol{
                    .name_offset = 0,
                    .binding = 0,
                    .type = ELFSymbolType::SECTION,
                    .section_index = 1,
                    .value = 0,
                    .size = 0,
                },
                ELFSymbol{
                    .name_offset = 0,
                    .binding = 0,
                    .type = ELFSymbolType::SECTION,
                    .section_index = 2,
                    .value = 0,
                    .size = 0,
                },
            },
    };

    text_rela_section = ELFSection{
        .name_offset = add_string(shstrtab_section, ".rela.text"),
        .type = ELFSectionType::RELA,
        .link = 5,
        .info = 1,
        .alignment = 8,
        .entry_size = 24,
        .data = ELFSection::RelocationList{},
    };

    data_rela_section = ELFSection{
        .name_offset = add_string(shstrtab_section, ".rela.data"),
        .type = ELFSectionType::RELA,
        .link = 5,
        .info = 2,
        .alignment = 8,
        .entry_size = 24,
        .data = ELFSection::RelocationList{},
    };

    if (module_.bnjatbl_data) {
        bnjatbl_section = ELFSection{
            .name_offset = add_string(shstrtab_section, ".bnjatbl"),
            .type = ELFSectionType::PROGBITS,
            .flags = ELFSectionFlags::ALLOC | ELFSectionFlags::WRITE,
            .alignment = 16,
            .data = module_.bnjatbl_data->move_data(),
        };

        bnjatbl_rela_section = ELFSection{
            .name_offset = add_string(shstrtab_section, ".rela.bnjatbl"),
            .type = ELFSectionType::RELA,
            .link = 5,
            .info = 8,
            .alignment = 8,
            .entry_size = 24,
            .data = ELFSection::RelocationList{},
        };
    }

    process_defs(module_.symbol_defs);
    process_uses(module_.symbol_uses);

    file.sections = {
        std::move(text_section),
        std::move(data_section),
        std::move(shstrtab_section),
        std::move(strtab_section),
        std::move(symtab_section),
        std::move(text_rela_section),
        std::move(data_rela_section),
    };

    if (bnjatbl_section) {
        file.sections.push_back(std::move(*bnjatbl_section));
        file.sections.push_back(std::move(*bnjatbl_rela_section));
    }

    return file;
}

void ELFBuilder::process_defs(const std::vector<BinSymbolDef> &defs) {
    ELFSection::SymbolList &symbols = std::get<ELFSection::SymbolList>(symtab_section.data);
    elf_symbol_indices.resize(defs.size());

    // Process local symbol defintions.
    for (unsigned i = 0; i < defs.size(); i++) {
        const BinSymbolDef &def = defs[i];

        if (!def.global) {
            elf_symbol_indices[i] = symbols.size();
            process_def(def);
        }
    }

    // The info field of the symbol table is the index of the first non-local symbol.
    symtab_section.info = std::get<ELFSection::SymbolList>(symtab_section.data).size();

    // Process global symbol defintions.
    for (unsigned i = 0; i < defs.size(); i++) {
        const BinSymbolDef &def = defs[i];

        if (def.global) {
            elf_symbol_indices[i] = symbols.size();
            process_def(def);
        }
    }
}

void ELFBuilder::process_def(const BinSymbolDef &def) {
    ELFSection::SymbolList &symbols = std::get<ELFSection::SymbolList>(symtab_section.data);

    std::uint8_t type;
    std::uint16_t section_index;

    switch (def.kind) {
        case BinSymbolKind::TEXT_FUNC:
            type = ELFSymbolType::FUNC;
            section_index = 1;
            break;
        case BinSymbolKind::TEXT_LABEL:
            type = ELFSymbolType::NOTYPE;
            section_index = 1;
            break;
        case BinSymbolKind::DATA_LABEL:
            type = ELFSymbolType::OBJECT;
            section_index = 2;
            break;
        case BinSymbolKind::ADDR_TABLE:
            type = ELFSymbolType::NOTYPE;
            section_index = 8;
            break;
        case BinSymbolKind::UNKNOWN:
            type = ELFSymbolType::NOTYPE;
            section_index = 0;
            break;
    }

    symbols.push_back(ELFSymbol{
        .name_offset = add_string(strtab_section, def.name),
        .binding = def.global ? ELFSymbolBinding::GLOBAL : ELFSymbolBinding::LOCAL,
        .type = type,
        .section_index = section_index,
        .value = def.offset,
        .size = 0,
    });
}

void ELFBuilder::process_uses(const std::vector<BinSymbolUse> &uses) {
    for (const BinSymbolUse &use : uses) {
        ELFSection *section;

        switch (use.section) {
            case BinSectionKind::TEXT: section = &text_rela_section; break;
            case BinSectionKind::DATA: section = &data_rela_section; break;
            case BinSectionKind::BNJATBL: section = &*bnjatbl_rela_section; break;
        }

        int address_offset;
        std::uint32_t type;

        switch (use.kind) {
            case BinSymbolUseKind::ABS64:
                address_offset = 0;
                type = ELFRelocationType::X86_64_64;
                break;
            case BinSymbolUseKind::REL32:
                address_offset = -4;
                type = ELFRelocationType::X86_64_PC32;
                break;
            case BinSymbolUseKind::GOTPCREL32:
                address_offset = -4;
                type = ELFRelocationType::X86_64_GOTPCREL;
                break;
            case BinSymbolUseKind::PLT32:
                address_offset = -4;
                type = ELFRelocationType::X86_64_PLT32;
                break;
        }

        ELFSection::RelocationList &relocations = std::get<ELFSection::RelocationList>(section->data);

        relocations.push_back(ELFRelocation{
            .offset = use.address,
            .symbol_index = elf_symbol_indices[use.symbol_index],
            .type = type,
            .addend = use.addend + address_offset,
        });
    }
}

std::uint32_t ELFBuilder::add_string(ELFSection &section, std::string_view string) {
    ELFSection::BinaryData &data = std::get<ELFSection::BinaryData>(section.data);
    std::uint32_t offset = data.size();

    for (char c : string) {
        data.push_back(c);
    }
    data.push_back('\0');

    return offset;
}

} // namespace banjo
