#ifndef ELF_FORMAT_H
#define ELF_FORMAT_H

#include <cstdint>
#include <variant>
#include <vector>

namespace banjo {

namespace ELFSectionType {
enum : std::uint32_t {
    PROGBITS = 1,
    SYMTAB = 2,
    STRTAB = 3,
    RELA = 4,
};
}

namespace ELFSectionFlags {
enum : std::uint64_t {
    WRITE = 0x1,
    ALLOC = 0x2,
    EXECINSTR = 0x4,
};
}

namespace ELFSymbolBinding {
enum : std::uint8_t {
    LOCAL = 0,
    GLOBAL = 1,
};
};

namespace ELFSymbolType {
enum : std::uint8_t {
    NOTYPE = 0,
    OBJECT = 1,
    FUNC = 2,
    SECTION = 3,
};
};

struct ELFSymbol {
    std::uint32_t name_offset = 0;
    std::uint8_t binding;
    std::uint8_t type;
    std::uint16_t section_index;
    std::uint64_t value;
    std::uint64_t size = 0;
};

namespace ELFRelocationType {
enum : std::uint32_t {
    X86_64_64 = 1,
    X86_64_PC32 = 2,
    X86_64_GOT32 = 3,
    X86_64_PLT32 = 4,
    X86_64_GOTPCREL = 5,
};
}

struct ELFRelocation {
    std::uint64_t offset;
    std::uint32_t symbol_index;
    std::uint32_t type;
    std::int64_t addend;
};

struct ELFSection {
    typedef std::vector<std::uint8_t> BinaryData;
    typedef std::vector<ELFSymbol> SymbolList;
    typedef std::vector<ELFRelocation> RelocationList;
    typedef std::variant<BinaryData, SymbolList, RelocationList> Data;

    std::uint32_t name_offset;
    std::uint32_t type;
    std::uint64_t flags = 0;
    std::uint32_t link = 0;
    std::uint32_t info = 0;
    std::uint64_t alignment;
    std::uint64_t entry_size = 0;
    Data data;
};

namespace ELFMachine {
enum : std::uint16_t {
    X86_64 = 62,
    AARCH64 = 183,
};
};

struct ELFFile {
    std::uint16_t machine;
    std::vector<ELFSection> sections;
};

} // namespace banjo

#endif
