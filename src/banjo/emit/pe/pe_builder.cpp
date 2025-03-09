#include "pe_builder.hpp"
#include "banjo/emit/pe/pe_format.hpp"
#include <cstring>

namespace banjo {

PEFile PEBuilder::build(BinModule module_) {
    create_sections(module_);
    create_section_symbols();

    for (const BinSymbolDef &def : module_.symbol_defs) {
        process_x86_64_symbol_def(def);
    }

    for (const BinSymbolUse &use : module_.symbol_uses) {
        process_x86_64_symbol_use(use, module_);
    }

    create_unwind_info(module_.unwind_info);
    move_section_data(module_);

    return file;
}

void PEBuilder::create_sections(BinModule &module_) {
    using namespace PESectionFlags;

    // TODO: Calculate these alignments dynamically.

    file.sections = {
        PESection{
            .name = {'.', 't', 'e', 'x', 't', '\0', '\0', '\0'},
            .data = {},
            .relocations = {},
            .flags = CODE | ALIGN_16BYTES | EXECUTE | READ,
        },
        PESection{
            .name = {'.', 'd', 'a', 't', 'a', '\0', '\0', '\0'},
            .data = {},
            .relocations = {},
            .flags = INITIALIZED_DATA | ALIGN_16BYTES | READ | WRITE,
        },
        PESection{
            .name = {'.', 'p', 'd', 'a', 't', 'a', '\0', '\0'},
            .data = {},
            .relocations = {},
            .flags = INITIALIZED_DATA | ALIGN_4BYTES | READ,
        },
        PESection{
            .name = {'.', 'x', 'd', 'a', 't', 'a', '\0', '\0'},
            .data = {},
            .relocations = {},
            .flags = INITIALIZED_DATA | ALIGN_4BYTES | READ,
        },
    };

    if (module_.drectve_data) {
        drectve_section_index = file.sections.size();

        file.sections.push_back(PESection{
            .name = {'.', 'd', 'r', 'e', 'c', 't', 'v', 'e'},
            .data = {},
            .relocations = {},
            .flags = LNK_INFO | LNK_REMOVE | ALIGN_1BYTES,
        });
    }

    if (module_.bnjatbl_data) {
        bnjatbl_section_index = file.sections.size();

        file.sections.push_back(PESection{
            .name = {'.', 'b', 'n', 'j', 'a', 't', 'b', 'l'},
            .data = {},
            .relocations = {},
            .flags = INITIALIZED_DATA | ALIGN_16BYTES | READ | WRITE,
        });
    }
}

void PEBuilder::create_section_symbols() {
    for (unsigned i = 0; i < file.sections.size(); i++) {
        PESection &section = file.sections[i];

        std::string name(8, '\0');
        std::memcpy(name.data(), section.name, 8);

        file.add_symbol({
            .name = name,
            .value = 0,
            .section_number = get_section_number(i),
            .storage_class = PEStorageClass::STATIC,
        });
    }

    num_section_symbols = file.sections.size();
}

void PEBuilder::process_x86_64_symbol_def(const BinSymbolDef &def) {
    std::int16_t section_number;
    switch (def.kind) {
        case BinSymbolKind::TEXT_FUNC: section_number = get_section_number(TEXT_SECTION_INDEX); break;
        case BinSymbolKind::TEXT_LABEL: section_number = get_section_number(TEXT_SECTION_INDEX); break;
        case BinSymbolKind::DATA_LABEL: section_number = get_section_number(DATA_SECTION_INDEX); break;
        case BinSymbolKind::ADDR_TABLE: section_number = get_section_number(bnjatbl_section_index); break;
        case BinSymbolKind::UNKNOWN: section_number = 0; break;
    }

    file.add_symbol(PESymbolBlueprint{
        .name = def.name,
        .value = def.offset,
        .section_number = section_number,
        .storage_class = def.global ? PEStorageClass::EXTERNAL : PEStorageClass::STATIC
    });
}

void PEBuilder::process_x86_64_symbol_use(const BinSymbolUse &use, BinModule &module_) {
    const BinSymbolDef def = module_.symbol_defs[use.symbol_index];

    if (use.section == BinSectionKind::TEXT) {
        if (def.kind == BinSymbolKind::DATA_LABEL) {
            module_.text.seek(use.address);
            module_.text.write_i32(def.offset + use.addend);

            file.sections[TEXT_SECTION_INDEX].relocations.push_back(PERelocation{
                .virt_addr = use.address,
                .symbol_index = get_section_symbol_index(DATA_SECTION_INDEX),
                .type = PERelocationType::AMD64_REL32
            });
        } else {
            file.sections[TEXT_SECTION_INDEX].relocations.push_back(PERelocation{
                .virt_addr = use.address,
                .symbol_index = use.symbol_index + num_section_symbols,
                .type = PERelocationType::AMD64_REL32
            });
        }
    } else if (use.section == BinSectionKind::DATA) {
        module_.data.seek(use.address);
        module_.data.write_i64(0);

        file.sections[DATA_SECTION_INDEX].relocations.push_back(PERelocation{
            .virt_addr = use.address,
            .symbol_index = use.symbol_index + num_section_symbols,
            .type = PERelocationType::AMD64_ADDR64
        });
    } else if (use.section == BinSectionKind::BNJATBL) {
        module_.bnjatbl_data->seek(use.address);
        module_.bnjatbl_data->write_i64(0);

        file.sections[bnjatbl_section_index].relocations.push_back(PERelocation{
            .virt_addr = use.address,
            .symbol_index = use.symbol_index + num_section_symbols,
            .type = PERelocationType::AMD64_ADDR64
        });
    }
}

void PEBuilder::create_unwind_info(const std::vector<BinUnwindInfo> &unwind_info) {
    PESection &pdata = file.sections[PDATA_SECTION_INDEX];
    PESection &xdata = file.sections[XDATA_SECTION_INDEX];

    WriteBuffer pdata_buf;
    WriteBuffer xdata_buf;

    for (const BinUnwindInfo &frame_info : unwind_info) {
        // function start address
        pdata.relocations.push_back(PERelocation{
            .virt_addr = (std::uint32_t)pdata_buf.get_size(),
            .symbol_index = get_section_symbol_index(TEXT_SECTION_INDEX),
            .type = PERelocationType::AMD64_ADDR32NB
        });
        pdata_buf.write_i32(frame_info.start_addr);

        // function end address
        pdata.relocations.push_back(PERelocation{
            .virt_addr = (std::uint32_t)pdata_buf.get_size(),
            .symbol_index = get_section_symbol_index(TEXT_SECTION_INDEX),
            .type = PERelocationType::AMD64_ADDR32NB
        });
        pdata_buf.write_i32(frame_info.end_addr);

        // unwind info address
        pdata.relocations.push_back(PERelocation{
            .virt_addr = (std::uint32_t)pdata_buf.get_size(),
            .symbol_index = get_section_symbol_index(XDATA_SECTION_INDEX),
            .type = PERelocationType::AMD64_ADDR32NB
        });
        pdata_buf.write_i32(xdata_buf.get_size());

        std::uint32_t alloca_offset = frame_info.alloca_instr_end - frame_info.start_addr;

        xdata_buf.write_u8(1);             // version and flags (current version, no flags)
        xdata_buf.write_u8(alloca_offset); // size of prolog
        unsigned num_slots_pos = xdata_buf.tell();
        xdata_buf.write_u8(0); // number of unwind code slots
        xdata_buf.write_u8(0); // frame register and frame register offset (none)

        xdata_buf.write_u8(alloca_offset); // offset

        std::uint8_t operation_code = 1;
        // std::uint8_t operation_info = (frame_info.alloca_size - 8) / 8;
        std::uint8_t operation_info = 1;
        xdata_buf.write_u8((operation_info << 4) | operation_code); // operation code + info

        xdata_buf.write_u8(frame_info.alloca_size);
        xdata_buf.write_u8(frame_info.alloca_size >> 8);
        xdata_buf.write_u8(frame_info.alloca_size >> 16);
        xdata_buf.write_u8(frame_info.alloca_size >> 24);

        unsigned num_slots = 3;

        for (const BinPushedRegInfo &pushed_reg : frame_info.pushed_regs) {
            xdata_buf.write_u8(pushed_reg.instr_end);
            std::uint8_t operation_code = 0;
            std::uint8_t operation_info = pushed_reg.reg;
            xdata_buf.write_u8((operation_info << 4) | operation_code); // operation code + info
            num_slots += 1;
        }

        if (num_slots % 2 == 0) {
            xdata_buf.write_u8(0);
            xdata_buf.write_u8(0);
        }

        unsigned pos = xdata_buf.tell();
        xdata_buf.seek(num_slots_pos);
        xdata_buf.write_u8(num_slots);
        xdata_buf.seek(pos);
    }

    pdata.data = pdata_buf.move_data();
    xdata.data = xdata_buf.move_data();
}

void PEBuilder::create_debug_info() {
    // structure taken from: https://lists.llvm.org/pipermail/llvm-dev/2015-October/091847.html
}

void PEBuilder::move_section_data(BinModule &module_) {
    file.sections[TEXT_SECTION_INDEX].data = module_.text.move_data();
    file.sections[DATA_SECTION_INDEX].data = module_.data.move_data();

    if (module_.drectve_data) {
        file.sections[drectve_section_index].data = module_.drectve_data->move_data();
    }

    if (module_.bnjatbl_data) {
        file.sections[bnjatbl_section_index].data = module_.bnjatbl_data->move_data();
    }
}

std::int16_t PEBuilder::get_section_number(unsigned section_index) {
    // The section number is a one-based index into the section table.
    return static_cast<std::int16_t>(section_index) + 1;
}

std::uint32_t PEBuilder::get_section_symbol_index(unsigned section_index) {
    return static_cast<std::uint32_t>(section_index);
}

} // namespace banjo
