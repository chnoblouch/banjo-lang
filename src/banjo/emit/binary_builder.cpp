#include "binary_builder.hpp"

#include <cstdlib>
#include <iostream>

BinModule BinaryBuilder::create_module() {
    BinModule module_;

    std::uint32_t bin_index = 0;
    for (SymbolDef &def : defs) {
        if (def.kind == BinSymbolKind::UNKNOWN) {
            def.bin_offset = 0;
        } else {
            const std::vector<DataSlice> &slice_list =
                def.kind == BinSymbolKind::DATA_LABEL ? data_slices : text_slices;
            const DataSlice &slice = slice_list[def.slice_index];
            def.bin_offset = slice.offset + def.local_offset;
        }

        if (def.kind != BinSymbolKind::TEXT_LABEL) {
            def.bin_index = bin_index++;
        }
    }

    for (DataSlice &text_slice : text_slices) {
        module_.text.write_data(std::move(text_slice.buffer));

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
                .section = BinSectionKind::TEXT
            });
        }
    }

    for (DataSlice &data_slice : data_slices) {
        module_.data.write_data(std::move(data_slice.buffer));

        for (SymbolUse &use : data_slice.uses) {
            SymbolDef &def = defs[use.index];

            module_.symbol_uses.push_back(BinSymbolUse{
                .address = data_slice.offset + use.local_offset,
                .addend = use.addend,
                .symbol_index = def.bin_index,
                .kind = use.kind,
                .section = BinSectionKind::DATA
            });
        }
    }

    for (SymbolDef &def : defs) {
        if (def.kind == BinSymbolKind::TEXT_LABEL) {
            continue;
        }

        module_.symbol_defs.push_back(
            BinSymbolDef{.name = std::move(def.name), .kind = def.kind, .offset = def.bin_offset, .global = def.global}
        );
    }

    for (UnwindInfo &frame_info : unwind_info) {
        std::vector<BinPushedRegInfo> bin_pushed_regs;
        for (const PushedRegInfo &pushed_reg : frame_info.pushed_regs) {
            bin_pushed_regs.insert(
                bin_pushed_regs.begin(),
                BinPushedRegInfo{.reg = pushed_reg.reg, .instr_end = defs[pushed_reg.end_label].bin_offset}
            );
        }

        module_.unwind_info.push_back(BinUnwindInfo{
            .start_addr = defs[frame_info.start_symbol_index].bin_offset,
            .end_addr = defs[frame_info.end_symbol_index].bin_offset,
            .alloca_size = frame_info.alloca_size,
            .alloca_instr_start = defs[frame_info.alloca_start_label].bin_offset,
            .alloca_instr_end = defs[frame_info.alloca_end_label].bin_offset,
            .pushed_regs = std::move(bin_pushed_regs)
        });
    }

    return module_;
}

void BinaryBuilder::compute_slice_offsets() {
    std::uint32_t address = 0;
    for (DataSlice &slice : text_slices) {
        slice.offset = address;
        address += slice.buffer.get_size();
    }

    address = 0;
    for (DataSlice &slice : data_slices) {
        slice.offset = address;
        address += slice.buffer.get_size();
    }
}

void BinaryBuilder::create_text_slice() {
    text_slices.push_back(DataSlice{});
}

void BinaryBuilder::add_func_symbol(std::string name, mcode::Module &machine_module) {
    add_symbol_def(SymbolDef{
        .name = name,
        .kind = BinSymbolKind::TEXT_FUNC,
        //.global = machine_module.get_global_symbols().contains(name)
        .global = true
    });
}

void BinaryBuilder::add_label_symbol(std::string name) {
    add_symbol_def(SymbolDef{.name = name, .kind = BinSymbolKind::TEXT_LABEL, .global = false});
}

void BinaryBuilder::add_data_symbol(std::string name, mcode::Module &machine_module) {
    add_symbol_def(SymbolDef{
        .name = name,
        .kind = BinSymbolKind::DATA_LABEL,
        .global = machine_module.get_global_symbols().contains(name),
        .slice_index = (std::uint32_t)data_slices.size() - 1,
        .local_offset = (std::uint32_t)data_slices.back().buffer.get_size()
    });
}

void BinaryBuilder::add_symbol_def(SymbolDef def) {
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
        .addend = addend
    });
}

void BinaryBuilder::add_data_symbol_use(const std::string &symbol) {
    data_slices.back().uses.push_back(
        SymbolUse{.index = symbol_indices[symbol], .local_offset = (std::uint32_t)data_slices.back().buffer.get_size()}
    );
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

void BinaryBuilder::assert_condition(bool condition, std::string message) {
#ifndef NDEBUG
    if (!condition) {
        abort(message);
    }
#endif
}

void BinaryBuilder::abort(std::string message) {
    std::cerr << "error: " << message << std::endl;
    std::exit(1);
}
