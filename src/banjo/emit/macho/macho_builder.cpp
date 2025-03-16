#include "macho_builder.hpp"

#include "banjo/emit/macho/macho_format.hpp"

namespace banjo {

MachOFile MachOBuilder::build() {
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
                        .data{
                            // add w0, w0, w1
                            0x00,
                            0x00,
                            0x01,
                            0x0B,
                            // ret
                            0xC0,
                            0x03,
                            0x5F,
                            0xD6,
                            // add w0, w0, w0
                            0x00,
                            0x00,
                            0x00,
                            0x0B,
                            // ret
                            0xC0,
                            0x03,
                            0x5F,
                            0xD6,
                        },
                        .flags = MachOSectionFlags::SOME_INSTRUCTIONS | MachOSectionFlags::PURE_INSTRUCTIONS,
                    },
                },
            },
            MachOSymtabCommand{
                .symbols{
                    MachOSymbol{
                        .name = "_add",
                        .external = true,
                        .section_number = 1,
                        .value = 0x00,
                    },
                    MachOSymbol{
                        .name = "_times_two",
                        .external = true,
                        .section_number = 1,
                        .value = 0x08,
                    },
                },
            },
            MachODysymtabCommand {
                .local_symbols{
                    .index = 0,
                    .count = 0,
                },
                .external_symbols{
                    .index = 0,
                    .count = 2,
                },
                .undefined_symbols{
                    .index = 2,
                    .count = 0,
                },
            },
        },
    };
}

} // namespace banjo
