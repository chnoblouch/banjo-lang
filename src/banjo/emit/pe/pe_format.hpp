#ifndef PE_FORMAT_H
#define PE_FORMAT_H

#include <cstdint>
#include <string>
#include <vector>

namespace PESectionFlags {
enum : std::uint32_t {
    CODE = 0x00000020,
    LNK_INFO = 0x00000200,
    LNK_REMOVE = 0x00000800,
    INITIALIZED_DATA = 0x00000040,
    ALIGN_1BYTES = 0x00100000,
    ALIGN_4BYTES = 0x00300000,
    ALIGN_16BYTES = 0x00500000,
    EXECUTE = 0x20000000,
    READ = 0x40000000,
    WRITE = 0x80000000
};
}

enum class PERelocationType : std::uint16_t {
    AMD64_ADDR64 = 0x0001,
    AMD64_ADDR32 = 0x0002,
    AMD64_ADDR32NB = 0x0003,
    AMD64_REL32 = 0x0004
};

struct PERelocation {
    std::uint32_t virt_addr;
    std::uint32_t symbol_index;
    PERelocationType type;
};

struct PESection {
    std::uint8_t name[8];
    std::vector<std::uint8_t> data;
    std::vector<PERelocation> relocations;
    std::uint32_t flags;
};

enum class PEStorageClass : std::uint8_t { EXTERNAL = 2, STATIC = 3, FUNCTION = 101, FILE = 103 };

struct PESymbol {
    std::uint64_t name_data;
    std::uint32_t value;
    std::int16_t section_number;
    PEStorageClass storage_class;
};

struct PESymbolBlueprint {
    std::string name;
    std::uint32_t value;
    std::int16_t section_number;
    PEStorageClass storage_class;
};

struct PEStringTable {
    std::vector<std::string> strings;
    std::uint32_t size = 4; // the string table size includes the 4 bytes that indicate the size itself
};

struct PEFile {
    std::vector<PESection> sections;
    std::vector<PESymbol> symbols;
    PEStringTable string_table;

    void add_symbol(PESymbolBlueprint blueprint);
};

#endif
