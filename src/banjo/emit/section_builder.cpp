#include "banjo/emit/section_builder.hpp"

#include "banjo/emit/binary_builder.hpp"
#include "banjo/emit/binary_builder_symbol.hpp"

#include <utility>

namespace banjo {

SectionBuilder::SectionBuilder(BinaryBuilder &bin_builder, BinSectionKind kind) : bin_builder(bin_builder), kind(kind) {
    slices.push_back(SectionSlice{});
}

void SectionBuilder::add_symbol_def(std::string name, BinSymbolKind kind, bool global) {
    bin_builder.add_symbol_def(
        SymbolDef{
            .name = std::move(name),
            .kind = kind,
            .global = global,
            .slice_index = static_cast<std::uint32_t>(slices.size() - 1),
            .local_offset = static_cast<std::uint32_t>(slices.back().buffer.get_size()),
        }
    );
}

void SectionBuilder::attach_symbol_def(std::uint32_t index) {
    SymbolDef &def = bin_builder.defs[index];
    def.slice_index = static_cast<std::uint32_t>(slices.size() - 1);
    def.local_offset = static_cast<std::uint32_t>(slices.back().buffer.get_size());
}

void SectionBuilder::add_symbol_use(std::uint32_t index, BinSymbolUseKind kind, std::int32_t addend) {
    slices.back().uses.push_back(
        SymbolUse{
            .index = index,
            .local_offset = static_cast<std::uint32_t>(slices.back().buffer.get_size()),
            .kind = kind,
            .addend = addend,
        }
    );
}

void SectionBuilder::add_symbol_use(const std::string &name, BinSymbolUseKind kind, std::int32_t addend /*= 0*/) {
    add_symbol_use(bin_builder.symbol_indices.at(name), kind, addend);
}

void SectionBuilder::create_relaxable_slice() {
    slices.push_back(
        SectionSlice{
            .relaxable_branch = true,
        }
    );
}

void SectionBuilder::end_relaxable_slice() {
    slices.push_back(SectionSlice{});
}

void SectionBuilder::push_out_slices(std::uint32_t starting_index, std::uint8_t offset /*= 0 */) {
    for (std::uint32_t i = starting_index; i < slices.size(); i++) {
        slices[i].offset += offset;
    }
}

void SectionBuilder::compute_slice_offsets() {
    std::uint32_t offset = 0;

    for (SectionSlice &slice : slices) {
        slice.offset = offset;
        offset += slice.buffer.get_size();
    }
}

WriteBuffer SectionBuilder::bake(std::vector<BinSymbolUse> &out_uses) {
    WriteBuffer buffer;

    for (SectionSlice &slice : slices) {
        buffer.write_data(slice.buffer);

        for (SymbolUse &use : slice.uses) {
            if (use.is_resolved) {
                continue;
            }

            SymbolDef &def = bin_builder.defs[use.index];

            out_uses.push_back(
                BinSymbolUse{
                    .address = slice.offset + use.local_offset,
                    .addend = use.addend,
                    .symbol_index = def.bin_index,
                    .kind = use.kind,
                    .section = kind,
                }
            );
        }
    }

    return buffer;
}

} // namespace banjo
