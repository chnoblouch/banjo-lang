#ifndef BANJO_EMIT_SECTION_BUILDER_H
#define BANJO_EMIT_SECTION_BUILDER_H

#include "banjo/emit/binary_builder_symbol.hpp"
#include "banjo/emit/binary_module.hpp"
#include "banjo/utils/write_buffer.hpp"

#include <vector>

namespace banjo {

class BinaryBuilder;

class SectionBuilder {

public:
    struct SectionSlice {
        std::vector<SymbolUse> uses;
        bool relaxable_branch = false;
        WriteBuffer buffer;
        std::uint32_t offset = 0;
    };

private:
    BinaryBuilder &bin_builder;

    BinSectionKind kind;
    std::vector<SectionSlice> slices;

public:
    SectionBuilder(BinaryBuilder &bin_builder, BinSectionKind kind);
    std::vector<SectionSlice> &get_slices() { return slices; }

    SectionSlice &cur_slice() { return slices.back(); }
    WriteBuffer &cur_buffer() { return cur_slice().buffer; }

    void write_u8(std::uint8_t u8) { cur_buffer().write_u8(u8); }
    void write_i8(std::int8_t i8) { cur_buffer().write_i8(i8); }
    void write_u16(std::uint16_t u16) { cur_buffer().write_u16(u16); }
    void write_i16(std::int32_t i16) { cur_buffer().write_i16(i16); }
    void write_u32(std::uint32_t u32) { cur_buffer().write_u32(u32); }
    void write_i32(std::int32_t i32) { cur_buffer().write_i32(i32); }
    void write_u64(std::uint64_t u64) { cur_buffer().write_u64(u64); }
    void write_i64(std::int64_t i64) { cur_buffer().write_i64(i64); }
    void write_f32(float f32) { cur_buffer().write_f32(f32); }
    void write_f64(double f64) { cur_buffer().write_f64(f64); }
    void write_data(const void *data, std::size_t size) { cur_buffer().write_data(data, size); }
    void write_data(const WriteBuffer &buffer) { cur_buffer().write_data(buffer); }
    void write_zeroes(std::size_t size) { cur_buffer().write_zeroes(size); }
    void write_cstr(const char *cstr) { cur_buffer().write_cstr(cstr); }

    void add_symbol_def(std::string name, BinSymbolKind kind, bool global);
    void attach_symbol_def(std::uint32_t index);
    void add_symbol_use(std::uint32_t index, BinSymbolUseKind kind, std::int32_t addend = 0);
    void add_symbol_use(const std::string &name, BinSymbolUseKind kind, std::int32_t addend = 0);

    void create_relaxable_slice();
    void end_relaxable_slice();
    void push_out_slices(unsigned starting_index, std::uint8_t offset);

    void compute_slice_offsets();
    WriteBuffer bake(std::vector<BinSymbolUse> &out_uses);
};

}; // namespace banjo

#endif
