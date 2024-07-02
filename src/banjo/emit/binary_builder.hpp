#ifndef BINARY_BUILDER_H
#define BINARY_BUILDER_H

#include "banjo/emit/binary_module.hpp"
#include "banjo/mcode/module.hpp"
#include "banjo/utils/write_buffer.hpp"

#include <unordered_map>

namespace banjo {

class BinaryBuilder {

protected:
    struct SymbolDef {
        std::string name;
        BinSymbolKind kind;
        bool global;
        std::uint32_t slice_index;
        std::uint32_t local_offset;

        std::uint32_t bin_index;
        std::uint32_t bin_offset;
    };

    struct SymbolUse {
        std::uint32_t index;
        std::uint32_t local_offset;
        BinSymbolUseKind kind;
        std::int32_t addend;
    };

    enum SliceType { OPAQUE, RELAXABLE, ALIGN16 };

    struct DataSlice {
        std::vector<SymbolUse> uses;
        bool relaxable_branch = false;
        WriteBuffer buffer;
        std::uint32_t offset;
    };

    struct PushedRegInfo {
        unsigned reg;
        std::uint32_t start_label;
        std::uint32_t end_label;
    };

    struct UnwindInfo {
        std::uint32_t start_symbol_index;
        std::uint32_t end_symbol_index;
        std::uint32_t alloca_size;
        std::uint32_t alloca_start_label;
        std::uint32_t alloca_end_label;
        std::vector<PushedRegInfo> pushed_regs;
    };

    std::vector<DataSlice> text_slices;
    std::vector<DataSlice> data_slices;
    std::vector<SymbolDef> defs;
    std::vector<UnwindInfo> unwind_info;
    std::unordered_map<std::string, std::uint32_t> symbol_indices;
    std::unordered_map<std::string, std::uint32_t> label_indices;

    BinModule create_module();
    void compute_slice_offsets();

    DataSlice &get_text_slice() { return text_slices.back(); }
    void create_text_slice();

    void add_func_symbol(std::string name, mcode::Module &machine_module);
    void add_label_symbol(std::string name);
    void add_data_symbol(std::string name, mcode::Module &machine_module);
    void add_symbol_def(SymbolDef def);
    void attach_symbol_def(std::uint32_t index);
    void add_text_symbol_use(const std::string &symbol, std::int32_t addend);
    void add_text_symbol_use(std::uint32_t symbol_index, BinSymbolUseKind kind, std::int32_t addend);
    void add_data_symbol_use(const std::string &symbol);
    std::uint32_t add_empty_label();
    void push_out_slices(unsigned starting_index, std::uint8_t offset);

    void assert_condition(bool condition, std::string message);
    [[noreturn]] void abort(std::string message);
};

} // namespace banjo

#endif
