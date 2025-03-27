#ifndef BINARY_MODULE
#define BINARY_MODULE

#include "banjo/utils/write_buffer.hpp"

#include <optional>
#include <string>
#include <vector>

namespace banjo {

enum class BinSymbolKind {
    TEXT_FUNC,
    TEXT_LABEL,
    DATA_LABEL,
    ADDR_TABLE,
    UNKNOWN,
};

struct BinSymbolDef {
    std::string name;
    BinSymbolKind kind;
    std::uint32_t offset;
    bool global;
};

enum class BinSymbolUseKind {
    INTERNAL,

    ABS64,
    REL32,
    GOTPCREL32,
    PLT32,

    // Relocations for AArch64 Mach-O
    BRANCH26,
    PAGE21,
    PAGEOFF12,
    GOT_LOAD_PAGE21,
    GOT_LOAD_PAGEOFF12,
};

enum class BinSectionKind {
    TEXT,
    DATA,
    BNJATBL,
};

struct BinSymbolUse {
    std::uint32_t address;
    std::int32_t addend;
    std::uint32_t symbol_index;
    BinSymbolUseKind kind;
    BinSectionKind section;
};

struct BinPushedRegInfo {
    unsigned reg;
    std::uint32_t instr_end;
};

struct BinUnwindInfo {
    std::uint32_t func_symbol_index;
    std::uint32_t start_addr;
    std::uint32_t end_addr;

    std::uint32_t alloca_size;
    std::uint32_t alloca_instr_start;
    std::uint32_t alloca_instr_end;
    std::vector<BinPushedRegInfo> pushed_regs;
};

struct BinModule {
    WriteBuffer text;
    WriteBuffer data;
    std::vector<BinSymbolDef> symbol_defs;
    std::vector<BinSymbolUse> symbol_uses;
    std::vector<BinUnwindInfo> unwind_info;
    std::optional<WriteBuffer> bnjatbl_data;

    // PE-specific variables:
    std::optional<WriteBuffer> drectve_data;

    // ELF-specific variables:
};

} // namespace banjo

#endif
