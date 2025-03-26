#ifndef BINARY_BUILDER_H
#define BINARY_BUILDER_H

#include "banjo/emit/binary_builder_symbol.hpp"
#include "banjo/emit/binary_module.hpp"
#include "banjo/mcode/module.hpp"
#include "section_builder.hpp"

#include <optional>
#include <unordered_map>

namespace banjo {

class BinaryBuilder {

    friend class SectionBuilder;

protected:
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

    SectionBuilder text;
    SectionBuilder data;
    std::optional<SectionBuilder> addr_table;

    std::vector<SymbolDef> defs;
    std::vector<UnwindInfo> unwind_info;
    std::unordered_map<std::string, std::uint32_t> symbol_indices;
    std::unordered_map<std::string, std::uint32_t> label_indices;

public:
    BinaryBuilder();
    BinModule encode(mcode::Module &m_mod);

private:
    virtual void encode_instr(mcode::Instruction &instr, mcode::Function *func, UnwindInfo &frame_info) = 0;
    virtual void apply_relaxation() {}
    virtual void resolve_internal_symbols() {}

protected:
    BinModule create_module();
    void compute_slice_offsets();

    void generate_external_symbols(mcode::Module &m_mod);
    void generate_data_slices(mcode::Module &m_mod);
    void generate_addr_table_slices(mcode::Module &m_mod);

    void add_func_symbol(std::string name, mcode::Module &m_mod);
    void add_label_symbol(std::string name);
    void add_symbol_def(const SymbolDef &def);
    std::uint32_t add_empty_label();

private:
    void bake_symbol_locations();
    void bake_symbol_defs(BinModule &module_);
    void bake_unwind_info(BinModule &module_);
};

} // namespace banjo

#endif
