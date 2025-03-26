#include "binary_builder.hpp"

#include "banjo/config/config.hpp"
#include "banjo/emit/binary_module.hpp"

#include <utility>

namespace banjo {

BinaryBuilder::BinaryBuilder() : text(*this, BinSectionKind::TEXT), data(*this, BinSectionKind::DATA) {}

BinModule BinaryBuilder::encode(mcode::Module &m_mod) {
    generate_external_symbols(m_mod);

    std::uint32_t first_text_symbol_index = defs.size();

    for (mcode::Function *func : m_mod.get_functions()) {
        add_func_symbol(func->get_name(), m_mod);

        for (mcode::BasicBlock &block : func->get_basic_blocks()) {
            if (!block.get_label().empty()) {
                add_label_symbol(block.get_label());
            }
        }

        add_empty_label();
    }

    generate_data_slices(m_mod);
    generate_addr_table_slices(m_mod);

    std::uint32_t symbol_index = first_text_symbol_index;

    for (mcode::Function *func : m_mod.get_functions()) {
        UnwindInfo frame_info{
            .start_symbol_index = symbol_index,
            .alloca_size = func->get_unwind_info().alloc_size,
        };

        text.attach_symbol_def(symbol_index++);

        for (mcode::BasicBlock &block : func->get_basic_blocks()) {
            if (!block.get_label().empty()) {
                text.attach_symbol_def(symbol_index++);
            }

            for (mcode::Instruction &instr : block) {
                encode_instr(instr, func, frame_info);
            }
        }

        frame_info.end_symbol_index = symbol_index;
        text.attach_symbol_def(symbol_index++);

        unwind_info.push_back(frame_info);
    }

    compute_slice_offsets();
    apply_relaxation();
    resolve_internal_symbols();

    return create_module();
}

BinModule BinaryBuilder::create_module() {
    BinModule bin_mod;
    bake_symbol_locations();

    bin_mod.text = text.bake(bin_mod.symbol_uses, {BinSymbolKind::TEXT_FUNC, BinSymbolKind::TEXT_LABEL});
    bin_mod.data = data.bake(bin_mod.symbol_uses, {});

    if (addr_table) {
        bin_mod.bnjatbl_data = addr_table->bake(bin_mod.symbol_uses, {});
    }

    bake_symbol_defs(bin_mod);
    bake_unwind_info(bin_mod);

    return bin_mod;
}

void BinaryBuilder::bake_symbol_locations() {
    std::uint32_t bin_index = 0;

    for (SymbolDef &def : defs) {
        if (def.kind == BinSymbolKind::UNKNOWN) {
            def.bin_offset = 0;
        } else {
            SectionBuilder &section = def.kind == BinSymbolKind::DATA_LABEL ? data : text;
            std::vector<SectionBuilder::SectionSlice> &slice_list = section.get_slices();
            SectionBuilder::SectionSlice &slice = slice_list[def.slice_index];
            def.bin_offset = slice.offset + def.local_offset;
        }

        if (def.kind != BinSymbolKind::TEXT_LABEL) {
            def.bin_index = bin_index++;
        }
    }
}

void BinaryBuilder::bake_symbol_defs(BinModule &module_) {
    for (SymbolDef &def : defs) {
        if (def.kind == BinSymbolKind::TEXT_LABEL) {
            continue;
        }

        std::string name;

        if (lang::Config::instance().target.is_darwin()) {
            // TODO: Move this somewhere else!
            name = "_" + std::move(def.name);
        } else {
            name = std::move(def.name);
        }

        module_.symbol_defs.push_back(
            BinSymbolDef{
                .name = name,
                .kind = def.kind,
                .offset = def.bin_offset,
                .global = def.global,
            }
        );
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

        module_.unwind_info.push_back(
            BinUnwindInfo{
                .start_addr = defs[frame_info.start_symbol_index].bin_offset,
                .end_addr = defs[frame_info.end_symbol_index].bin_offset,
                .alloca_size = frame_info.alloca_size,
                .alloca_instr_start = defs[frame_info.alloca_start_label].bin_offset,
                .alloca_instr_end = defs[frame_info.alloca_end_label].bin_offset,
                .pushed_regs = std::move(bin_pushed_regs),
            }
        );
    }
}

void BinaryBuilder::compute_slice_offsets() {
    text.compute_slice_offsets();
    data.compute_slice_offsets();

    if (addr_table) {
        addr_table->compute_slice_offsets();
    }
}

void BinaryBuilder::generate_external_symbols(mcode::Module &m_mod) {
    for (const std::string &external : m_mod.get_external_symbols()) {
        add_symbol_def(
            SymbolDef{
                .name = external,
                .kind = BinSymbolKind::UNKNOWN,
                .global = true,
            }
        );
    }
}

void BinaryBuilder::generate_data_slices(mcode::Module &m_mod) {
    for (const mcode::Global &global : m_mod.get_globals()) {
        ASSERT(global.alignment != 0);

        // Careful here: This alignment calculation will stop working if the data section is split
        // up into multiple slices!
        while (data.cur_buffer().get_size() % global.alignment != 0) {
            data.write_u8(0);
        }

        bool is_global = m_mod.get_global_symbols().contains(global.name);
        data.add_symbol_def(global.name, BinSymbolKind::DATA_LABEL, is_global);

        if (auto value = std::get_if<mcode::Global::Integer>(&global.value)) {
            switch (global.size) {
                case 1: data.write_u8(value->to_bits()); break;
                case 2: data.write_u16(value->to_bits()); break;
                case 4: data.write_u32(value->to_bits()); break;
                case 8: data.write_u64(value->to_bits()); break;
                default: ASSERT_UNREACHABLE;
            }
        } else if (auto value = std::get_if<mcode::Global::FloatingPoint>(&global.value)) {
            switch (global.size) {
                case 4: data.write_f32(*value); break;
                case 8: data.write_f64(*value); break;
                default: ASSERT_UNREACHABLE;
            }
        } else if (auto value = std::get_if<mcode::Global::Bytes>(&global.value)) {
            data.write_data(value->data(), value->size());
        } else if (auto value = std::get_if<mcode::Global::String>(&global.value)) {
            data.write_data(value->data(), value->size());
        } else if (auto value = std::get_if<mcode::Global::SymbolRef>(&global.value)) {
            data.add_symbol_use(value->name, BinSymbolUseKind::ABS64, 0);
            data.write_i64(0);
        } else if (std::holds_alternative<mcode::Global::None>(global.value)) {
            data.write_zeroes(global.size);
        } else {
            ASSERT_UNREACHABLE;
        }
    }
}

void BinaryBuilder::generate_addr_table_slices(mcode::Module &m_mod) {
    if (!m_mod.get_addr_table()) {
        return;
    }

    addr_table.emplace(*this, BinSectionKind::BNJATBL);

    addr_table->add_symbol_def("addr_table", BinSymbolKind::ADDR_TABLE, true);
    addr_table->write_u32(m_mod.get_addr_table()->entries.size());

    for (const std::string &symbol : m_mod.get_addr_table()->entries) {
        addr_table->write_u32(symbol.size());
        addr_table->write_cstr(symbol.c_str());
    }

    for (const std::string &symbol : m_mod.get_addr_table()->entries) {
        addr_table->add_symbol_use(symbol_indices.at(symbol), BinSymbolUseKind::ABS64, 0);
        addr_table->write_zeroes(8);
    }
}

void BinaryBuilder::add_func_symbol(std::string name, mcode::Module & /*m_mod*/) {
    // bool is_global = m_mod.get_global_symbols().contains(name);
    text.add_symbol_def(std::move(name), BinSymbolKind::TEXT_FUNC, true);
}

void BinaryBuilder::add_label_symbol(std::string name) {
    text.add_symbol_def(std::move(name), BinSymbolKind::TEXT_LABEL, false);
}

void BinaryBuilder::add_symbol_def(const SymbolDef &def) {
    symbol_indices.insert({def.name, defs.size()});
    defs.push_back(def);
}

std::uint32_t BinaryBuilder::add_empty_label() {
    add_label_symbol("");
    return defs.size() - 1;
}

} // namespace banjo
