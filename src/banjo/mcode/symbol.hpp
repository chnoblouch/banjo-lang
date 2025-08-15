#ifndef BANJO_MCODE_SYMBOL_H
#define BANJO_MCODE_SYMBOL_H

#include <cstdint>
#include <string>

namespace banjo {
namespace mcode {

enum class Relocation : std::uint8_t {
    NONE,

    // Common relocations
    GOT,
    PLT,

    // Relocations for AArch64 ELF
    ADR_PREL_PG_HI21,
    ADD_ABS_LO12_NC,
    ADR_GOT_PAGE,
    LD64_GOT_LO12_NC,

    // Relocations for AArch64 Mach-O
    PAGE21,
    PAGEOFF12,
    GOT_LOAD_PAGE21,
    GOT_LOAD_PAGEOFF12,

    // Relocations for WebAssembly
    MEMORY_ADDR_SLEB,
    TABLE_INDEX_SLEB,
};

struct Symbol {
    std::string name;
    Relocation reloc;

    Symbol(std::string name) : name(std::move(name)), reloc(Relocation::NONE) {}
    Symbol(std::string name, Relocation reloc) : name(std::move(name)), reloc(reloc) {}

    friend bool operator==(const Symbol &lhs, const Symbol &rhs) {
        return lhs.name == rhs.name && lhs.reloc == rhs.reloc;
    }

    friend bool operator!=(const Symbol &lhs, const Symbol &rhs) { return !(lhs == rhs); }
};

} // namespace mcode
} // namespace banjo

#endif
