#include "binary_builder.hpp"

#include <utility>

namespace banjo {

BinModule BinaryBuilder::create_module() {
    BinModule module_;
    bake_symbol_locations();
    merge_text_slices(module_);
    merge_data_slices(module_);
    bake_symbol_defs(module_);
    bake_unwind_info(module_);
    bake_addr_table(module_);
    return module_;
}

void BinaryBuilder::bake_symbol_locations() {
    std::uint32_t bin_index = 0;

    for (SymbolDef &def : defs) {
        if (def.kind == BinSymbolKind::UNKNOWN) {
            def.bin_offset = 0;
        } else {
            const std::vector<SectionSlice> &slice_list =
                def.kind == BinSymbolKind::DATA_LABEL ? data_slices : text_slices;
            const SectionSlice &slice = slice_list[def.slice_index];
            def.bin_offset = slice.offset + def.local_offset;
        }

        if (def.kind != BinSymbolKind::TEXT_LABEL) {
            def.bin_index = bin_index++;
        }
    }
}

void BinaryBuilder::merge_text_slices(BinModule &module_) {
    for (SectionSlice &text_slice : text_slices) {
        module_.text.write_data(text_slice.buffer);

        for (SymbolUse &use : text_slice.uses) {
            SymbolDef &def = defs[use.index];

            if (def.kind == BinSymbolKind::TEXT_LABEL) {
                continue;
            }

            module_.symbol_uses.push_back(BinSymbolUse{
                .address = text_slice.offset + use.local_offset,
                .addend = use.addend,
                .symbol_index = def.bin_index,
                .kind = use.kind,
                .section = BinSectionKind::TEXT,
            });
        }
    }
}

void BinaryBuilder::merge_data_slices(BinModule &module_) {
    for (SectionSlice &data_slice : data_slices) {
        module_.data.write_data(data_slice.buffer);

        for (SymbolUse &use : data_slice.uses) {
            SymbolDef &def = defs[use.index];

            module_.symbol_uses.push_back(BinSymbolUse{
                .address = data_slice.offset + use.local_offset,
                .addend = use.addend,
                .symbol_index = def.bin_index,
                .kind = use.kind,
                .section = BinSectionKind::DATA,
            });
        }
    }
}

void BinaryBuilder::bake_symbol_defs(BinModule &module_) {
    for (SymbolDef &def : defs) {
        if (def.kind == BinSymbolKind::TEXT_LABEL) {
            continue;
        }

        module_.symbol_defs.push_back(BinSymbolDef{
            .name = std::move(def.name),
            .kind = def.kind,
            .offset = def.bin_offset,
            .global = def.global,
        });
    }
}

void BinaryBuilder::bake_unwind_info(BinModule &module_) {
    for (UnwindInfo &frame_info : unwind_info) {
        std::vector<BinPushedRegInfo> bin_pushed_regs;

        for (const PushedRegInfo &pushed_reg : frame_info.pushed_regs) {
            bin_pushed_regs.insert(
                bin_pushed_regs.begin(),
                BinPushedRegInfo{
                    .reg = pushed_reg.reg,
                    .instr_end = defs[pushed_reg.end_label].bin_offset,
                }
            );
        }

        module_.unwind_info.push_back(BinUnwindInfo{
            .start_addr = defs[frame_info.start_symbol_index].bin_offset,
            .end_addr = defs[frame_info.end_symbol_index].bin_offset,
            .alloca_size = frame_info.alloca_size,
            .alloca_instr_start = defs[frame_info.alloca_start_label].bin_offset,
            .alloca_instr_end = defs[frame_info.alloca_end_label].bin_offset,
            .pushed_regs = std::move(bin_pushed_regs),
        });
    }
}

void BinaryBuilder::bake_addr_table(BinModule &module_) {
    if (!addr_table_slice) {
        return;
    }

    module_.bnjatbl_data = std::move(addr_table_slice->buffer);

    for (SymbolUse &use : addr_table_slice->uses) {
        SymbolDef &def = defs[use.index];

        module_.symbol_uses.push_back(BinSymbolUse{
            .address = addr_table_slice->offset + use.local_offset,
            .addend = use.addend,
            .symbol_index = def.bin_index,
            .kind = use.kind,
            .section = BinSectionKind::BNJATBL,
        });
    }
}

void BinaryBuilder::compute_slice_offsets() {
    std::uint32_t address = 0;
    for (SectionSlice &slice : text_slices) {
        slice.offset = address;
        address += slice.buffer.get_size();
    }

    address = 0;
    for (SectionSlice &slice : data_slices) {
        slice.offset = address;
        address += slice.buffer.get_size();
    }
}

void BinaryBuilder::generate_data_slices(mcode::Module &m_mod) {
    data_slices.push_back(SectionSlice{});

    for (const mcode::Global &global : m_mod.get_globals()) {
        add_symbol_def(SymbolDef{
            .name = global.name,
            .kind = BinSymbolKind::DATA_LABEL,
            .global = m_mod.get_global_symbols().contains(global.name),
            .slice_index = (std::uint32_t)data_slices.size() - 1,
            .local_offset = (std::uint32_t)data_slices.back().buffer.get_size(),
        });

        if (auto value = std::get_if<mcode::Global::Integer>(&global.value)) {
            switch (global.size) {
                case 1: data_slices.back().buffer.write_u8(value->to_bits()); break;
                case 2: data_slices.back().buffer.write_u16(value->to_bits()); break;
                case 4: data_slices.back().buffer.write_u32(value->to_bits()); break;
                case 8: data_slices.back().buffer.write_u64(value->to_bits()); break;
                default: ASSERT_UNREACHABLE;
            }
        } else if (auto value = std::get_if<mcode::Global::FloatingPoint>(&global.value)) {
            switch (global.size) {
                case 4: data_slices.back().buffer.write_f32(*value); break;
                case 8: data_slices.back().buffer.write_f64(*value); break;
                default: ASSERT_UNREACHABLE;
            }
        } else if (auto value = std::get_if<mcode::Global::Bytes>(&global.value)) {
            data_slices.back().buffer.write_data(value->data(), value->size());
        } else if (auto value = std::get_if<mcode::Global::String>(&global.value)) {
            data_slices.back().buffer.write_data(value->data(), value->size());
        } else if (auto value = std::get_if<mcode::Global::SymbolRef>(&global.value)) {
            add_data_symbol_use(value->name);

            // FIXME: sizes other than 64 bits?
            data_slices.back().buffer.write_i64(0);
        } else if (std::holds_alternative<mcode::Global::None>(global.value)) {
            data_slices.back().buffer.write_zeroes(global.size);
        } else {
            ASSERT_UNREACHABLE;
        }
    }
}

void BinaryBuilder::generate_addr_table_slices(mcode::Module &m_mod) {
    if (!m_mod.get_addr_table()) {
        return;
    }

    addr_table_slice = SectionSlice{};
    WriteBuffer &buffer = addr_table_slice->buffer;

    add_symbol_def(SymbolDef{
        .name = "addr_table",
        .kind = BinSymbolKind::ADDR_TABLE,
        .global = true,
        .slice_index = 0,
        .local_offset = 0,
    });

    buffer.write_u32(m_mod.get_addr_table()->entries.size());

    for (const std::string &symbol : m_mod.get_addr_table()->entries) {
        buffer.write_u32(symbol.size());
        buffer.write_cstr(symbol.c_str());
    }

    for (const std::string &symbol : m_mod.get_addr_table()->entries) {
        addr_table_slice->uses.push_back(SymbolUse{
            .index = symbol_indices.at(symbol),
            .local_offset = static_cast<std::uint32_t>(buffer.get_size()),
        });

        buffer.write_zeroes(8);
    }
}

void BinaryBuilder::create_text_slice() {
    text_slices.push_back(SectionSlice{});
}

void BinaryBuilder::add_func_symbol(std::string name, mcode::Module & /* machine_module */) {
    add_symbol_def(SymbolDef{
        .name = std::move(name),
        .kind = BinSymbolKind::TEXT_FUNC,
        //.global = machine_module.get_global_symbols().contains(name)
        .global = true
    });
}

void BinaryBuilder::add_label_symbol(std::string name) {
    add_symbol_def(SymbolDef{
        .name = std::move(name),
        .kind = BinSymbolKind::TEXT_LABEL,
        .global = false,
    });
}

void BinaryBuilder::add_symbol_def(const SymbolDef &def) {
    symbol_indices.insert({def.name, defs.size()});
    defs.push_back(def);
}

void BinaryBuilder::attach_symbol_def(std::uint32_t index) {
    SymbolDef &def = defs[index];
    def.slice_index = (std::uint32_t)text_slices.size() - 1;
    def.local_offset = (std::uint32_t)text_slices.back().buffer.get_size();
}

void BinaryBuilder::add_text_symbol_use(const std::string &symbol, std::int32_t addend) {
    add_text_symbol_use(symbol_indices[symbol], BinSymbolUseKind::REL32, addend);
}

void BinaryBuilder::add_text_symbol_use(std::uint32_t symbol_index, BinSymbolUseKind kind, std::int32_t addend) {
    text_slices.back().uses.push_back(SymbolUse{
        .index = symbol_index,
        .local_offset = (std::uint32_t)text_slices.back().buffer.get_size(),
        .kind = kind,
        .addend = addend,
    });
}

void BinaryBuilder::add_data_symbol_use(const std::string &symbol) {
    data_slices.back().uses.push_back(SymbolUse{
        .index = symbol_indices[symbol],
        .local_offset = (std::uint32_t)data_slices.back().buffer.get_size(),
    });
}

std::uint32_t BinaryBuilder::add_empty_label() {
    add_label_symbol("");
    attach_symbol_def(defs.size() - 1);
    return defs.size() - 1;
}

void BinaryBuilder::push_out_slices(std::uint32_t starting_index, std::uint8_t offset) {
    for (std::uint32_t i = starting_index; i < text_slices.size(); i++) {
        text_slices[i].offset += offset;
    }
}

} // namespace banjo
