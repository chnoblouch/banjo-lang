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

    struct SectionSlice {
        std::vector<SymbolUse> uses;
        bool relaxable_branch = false;
        WriteBuffer buffer;
        std::uint32_t offset = 0;
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

    std::vector<SectionSlice> text_slices;
    std::vector<SectionSlice> data_slices;
    std::vector<SymbolDef> defs;
    std::vector<UnwindInfo> unwind_info;
    std::optional<SectionSlice> addr_table_slice;
    std::unordered_map<std::string, std::uint32_t> symbol_indices;
    std::unordered_map<std::string, std::uint32_t> label_indices;

    BinModule create_module();
    void compute_slice_offsets();

    void generate_data_slices(mcode::Module &m_mod);
    void generate_addr_table_slices(mcode::Module &m_mod);

    SectionSlice &get_text_slice() { return text_slices.back(); }
    void create_text_slice();

    void add_func_symbol(std::string name, mcode::Module &machine_module);
    void add_label_symbol(std::string name);
    void add_symbol_def(const SymbolDef &def);
    void attach_symbol_def(std::uint32_t index);
    void add_text_symbol_use(const std::string &symbol, BinSymbolUseKind kind, std::int32_t addend);
    void add_text_symbol_use(std::uint32_t symbol_index, BinSymbolUseKind kind, std::int32_t addend);
    void add_data_symbol_use(const std::string &symbol);
    std::uint32_t add_empty_label();
    void push_out_slices(unsigned starting_index, std::uint8_t offset);

private:
    void bake_symbol_locations();
    void merge_text_slices(BinModule &module_);
    void merge_data_slices(BinModule &module_);
    void bake_symbol_defs(BinModule &module_);
    void bake_unwind_info(BinModule &module_);
    void bake_addr_table(BinModule &module_);
};

} // namespace banjo

#endif
