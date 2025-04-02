#include "macho_builder.hpp"

#include "banjo/emit/binary_module.hpp"
#include "banjo/emit/macho/macho_format.hpp"
#include "banjo/utils/macros.hpp"
#include "banjo/utils/utils.hpp"

namespace banjo {

MachOFile MachOBuilder::build(BinModule mod) {
    // TODO: Calculate these alignments dynamically.

    text_section = MachOSection{
        .name = "__text",
        .segment_name = "__TEXT",
        .address = 0,
        .data = mod.text.move_data(),
        .alignment = 4,
        .relocations = {},
        .type = MachOSectionType::REGULAR,
        .flags = MachOSectionFlags::SOME_INSTRUCTIONS | MachOSectionFlags::PURE_INSTRUCTIONS,
    };

    data_section = MachOSection{
        .name = "__data",
        .segment_name = "__DATA",
        .address = text_section.data.size(), // TODO: Is this the right way to calculate the address?
        .data = mod.data.move_data(),
        .alignment = 16,
        .relocations = {},
        .type = MachOSectionType::REGULAR,
        .flags = 0x00000000,
    };

    process_defs(mod.symbol_defs);
    process_uses(mod.symbol_uses);

    return MachOFile{
        .cpu_type = MachOCPUType::ARM64,
        .cpu_sub_type = MachOCPUSubType::ARM64_ALL,
        .load_commands{
            MachOSegment{
                .name = "",
                .maximum_vm_prot = MachOVMProt::READ | MachOVMProt::WRITE | MachOVMProt::EXECUTE,
                .initial_vm_prot = MachOVMProt::READ | MachOVMProt::WRITE | MachOVMProt::EXECUTE,
                .sections{
                    text_section,
                    data_section,
                },
            },
            MachOSymtabCommand{
                .symbols = symbols,
            },
            MachODysymtabCommand{
                .local_symbols{
                    .index = 0,
                    .count = num_local_symbols,
                },
                .external_symbols{
                    .index = num_local_symbols,
                    .count = num_external_symbols,
                },
                .undefined_symbols{
                    .index = num_local_symbols + num_external_symbols,
                    .count = num_undefined_symbols,
                },
            },
        },
    };
}

void MachOBuilder::process_defs(const std::vector<BinSymbolDef> &defs) {
    symbol_index_map.resize(defs.size());

    for (unsigned i = 0; i < defs.size(); i++) {
        const BinSymbolDef &def = defs[i];

        if (def.kind == BinSymbolKind::UNKNOWN || def.global) {
            continue;
        }

        process_def(def, i);
        num_local_symbols += 1;
    }

    for (unsigned i = 0; i < defs.size(); i++) {
        const BinSymbolDef &def = defs[i];

        if (def.kind == BinSymbolKind::UNKNOWN || !def.global) {
            continue;
        }

        process_def(def, i);
        num_external_symbols += 1;
    }

    for (unsigned i = 0; i < defs.size(); i++) {
        const BinSymbolDef &def = defs[i];

        if (def.kind != BinSymbolKind::UNKNOWN) {
            continue;
        }

        process_def(def, i);
        num_undefined_symbols += 1;
    }
}

void MachOBuilder::process_def(const BinSymbolDef &def, unsigned index) {
    std::uint8_t type;
    std::uint8_t section_number;
    std::uint64_t value;

    if (def.kind == BinSymbolKind::TEXT_FUNC) {
        type = MachOSymbolType::SECTION;
        section_number = 1;
        value = text_section.address + def.offset;
    } else if (def.kind == BinSymbolKind::DATA_LABEL) {
        type = MachOSymbolType::SECTION;
        section_number = 2;
        value = data_section.address + def.offset;
    } else {
        type = MachOSymbolType::UNDEFINED;
        section_number = 0;
        value = 0;
    }

    symbol_index_map[index] = symbols.size();

    symbols.push_back(
        MachOSymbol{
            .name = def.name,
            .type = type,
            .external = def.global,
            .section_number = section_number,
            .value = value,
        }
    );
}

void MachOBuilder::process_uses(const std::vector<BinSymbolUse> &uses) {
    for (const BinSymbolUse &use : uses) {
        process_use(use);
    }
}

void MachOBuilder::process_use(const BinSymbolUse &use) {
    bool pc_rel = Utils::is_one_of(
        use.kind,
        {
            BinSymbolUseKind::BRANCH26,
            BinSymbolUseKind::PAGE21,
            BinSymbolUseKind::GOT_LOAD_PAGE21,
        }
    );

    MachORelocation relocation{
        .address = static_cast<std::int32_t>(use.address),
        .value = symbol_index_map[use.symbol_index],
        .pc_rel = pc_rel,
        .length = static_cast<std::uint8_t>(use.kind == BinSymbolUseKind::ABS64 ? 8 : 4),
        .external = true,
        .type = to_relocation_type(use.kind),
    };

    if (use.section == BinSectionKind::TEXT) {
        text_section.relocations.push_back(relocation);
    } else if (use.section == BinSectionKind::DATA) {
        data_section.relocations.push_back(relocation);
    } else {
        ASSERT_UNREACHABLE;
    }
}

std::uint8_t MachOBuilder::to_relocation_type(BinSymbolUseKind use_kind) {
    switch (use_kind) {
        case BinSymbolUseKind::ABS64: return MachORelocationType::ARM64_UNSIGNED;
        case BinSymbolUseKind::BRANCH26: return MachORelocationType::ARM64_BRANCH26;
        case BinSymbolUseKind::PAGE21: return MachORelocationType::ARM64_PAGE21;
        case BinSymbolUseKind::PAGEOFF12: return MachORelocationType::ARM64_PAGEOFF12;
        case BinSymbolUseKind::GOT_LOAD_PAGE21: return MachORelocationType::ARM64_GOT_LOAD_PAGE21;
        case BinSymbolUseKind::GOT_LOAD_PAGEOFF12: return MachORelocationType::ARM64_GOT_LOAD_PAGEOFF12;
        default: ASSERT_UNREACHABLE;
    }
}

} // namespace banjo
