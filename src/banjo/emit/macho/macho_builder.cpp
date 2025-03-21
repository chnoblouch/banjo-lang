#include "macho_builder.hpp"

#include "banjo/emit/binary_module.hpp"
#include "banjo/emit/macho/macho_format.hpp"
#include "banjo/utils/macros.hpp"

namespace banjo {

MachOFile MachOBuilder::build(BinModule mod) {
    std::vector<MachOSymbol> symbols;
    std::vector<MachORelocation> relocations;
    std::uint32_t num_local_symbols = 0;
    std::uint32_t num_external_symbols = 0;

    for (BinSymbolDef &def : mod.symbol_defs) {
        std::uint8_t type;
        std::uint8_t section_number;

        if (def.kind == BinSymbolKind::TEXT_FUNC) {
            type = MachOSymbolType::SECTION;
            section_number = 1;
        } else {
            type = MachOSymbolType::UNDEFINED;
            section_number = 0;
        }

        symbols.push_back(
            MachOSymbol{
                .name = def.name,
                .type = type,
                .external = def.global,
                .section_number = section_number,
                .value = def.offset,
            }
        );

        if (def.global) {
            num_external_symbols += 1;
        } else {
            num_local_symbols += 1;
        }
    }

    for (BinSymbolUse &use : mod.symbol_uses) {
        std::uint8_t type;

        switch (use.kind) {
            case banjo::BinSymbolUseKind::BRANCH26: type = MachORelocationType::ARM64_BRANCH26; break;
            case banjo::BinSymbolUseKind::PAGE21: type = MachORelocationType::ARM64_PAGE21; break;
            case banjo::BinSymbolUseKind::PAGEOFF12: type = MachORelocationType::ARM64_PAGEOFF12; break;
            case banjo::BinSymbolUseKind::GOT_LOAD_PAGE21: type = MachORelocationType::ARM64_GOT_LOAD_PAGE21; break;
            case banjo::BinSymbolUseKind::GOT_LOAD_PAGEOFF12:
                type = MachORelocationType::ARM64_GOT_LOAD_PAGEOFF12;
                break;
            default: ASSERT_UNREACHABLE;
        }

        relocations.push_back(
            MachORelocation{
                .address = static_cast<std::int32_t>(use.address),
                .value = use.symbol_index,
                .pc_rel = true,
                .length = 4,
                .external = true,
                .type = type,
            }
        );
    }

    return MachOFile{
        .cpu_type = MachOCPUType::ARM64,
        .cpu_sub_type = MachOCPUSubType::ARM64_ALL,
        .load_commands{
            MachOSegment{
                .name = "",
                .maximum_vm_prot = MachOVMProt::READ | MachOVMProt::WRITE | MachOVMProt::EXECUTE,
                .initial_vm_prot = MachOVMProt::READ | MachOVMProt::WRITE | MachOVMProt::EXECUTE,
                .sections{
                    MachOSection{
                        .name = "__text",
                        .segment_name = "__TEXT",
                        .data = mod.text.move_data(),
                        .relocations = relocations,
                        .flags = MachOSectionFlags::SOME_INSTRUCTIONS | MachOSectionFlags::PURE_INSTRUCTIONS,
                    },
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
                    .count = 0,
                },
            },
        },
    };
}

} // namespace banjo
