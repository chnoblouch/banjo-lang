#include "macho_builder.hpp"

#include "banjo/emit/binary_module.hpp"
#include "banjo/emit/macho/macho_format.hpp"

namespace banjo {

MachOFile MachOBuilder::build(BinModule mod) {
    std::vector<MachOSymbol> symbols;
    std::uint32_t num_local_symbols = 0;
    std::uint32_t num_external_symbols = 0;

    for (BinSymbolDef &def : mod.symbol_defs) {
        if (def.kind != BinSymbolKind::TEXT_FUNC) {
            continue;
        }

        symbols.push_back(
            MachOSymbol{
                .name = def.name,
                .external = def.global,
                .section_number = 1,
                .value = def.offset,
            }
        );

        if (def.global) {
            num_external_symbols += 1;
        } else {
            num_local_symbols += 1;
        }
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
