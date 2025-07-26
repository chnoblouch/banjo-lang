#ifndef BANJO_EMIT_MACHO_FORMAT_H
#define BANJO_EMIT_MACHO_FORMAT_H

#include <cstdint>
#include <string>
#include <variant>
#include <vector>

namespace banjo {

namespace MachORelocationType {
enum : std::uint8_t {
    ARM64_UNSIGNED = 0,
    ARM64_BRANCH26 = 2,
    ARM64_PAGE21 = 3,
    ARM64_PAGEOFF12 = 4,
    ARM64_GOT_LOAD_PAGE21 = 5,
    ARM64_GOT_LOAD_PAGEOFF12 = 6,
};
}

struct MachORelocation {
    std::int32_t address;
    std::uint32_t value;
    bool pc_rel;
    std::uint8_t length;
    bool external;
    std::uint8_t type;
};

namespace MachOSectionType {
enum : std::uint32_t {
    REGULAR = 0x00,
};
}

namespace MachOSectionFlags {
enum : std::uint32_t {
    SOME_INSTRUCTIONS = 0x00000400,
    PURE_INSTRUCTIONS = 0x80000000,
};
}

struct MachOSection {
    std::string name;
    std::string segment_name;
    std::uint64_t address;
    std::vector<std::uint8_t> data;
    std::uint32_t alignment;
    std::vector<MachORelocation> relocations;
    std::uint32_t type;
    std::uint32_t flags;
};

namespace MachOVMProt {
enum {
    NONE = 0x00,
    READ = 0x01,
    WRITE = 0x02,
    EXECUTE = 0x04,
};
}

struct MachOSegment {
    char name[16];
    std::uint32_t maximum_vm_prot;
    std::uint32_t initial_vm_prot;
    std::vector<MachOSection> sections;
};

namespace MachOSymbolType {
enum : std::uint8_t {
    UNDEFINED = 0x00,
    SECTION = 0x0E,
};
};

struct MachOSymbol {
    std::string name;
    std::uint8_t type;
    bool external;
    std::uint8_t section_number;
    std::uint64_t value;
};

struct MachOSymtabCommand {
    std::vector<MachOSymbol> symbols;
};

struct MachODysymtabCommand {
    struct Group {
        std::uint32_t index;
        std::uint32_t count;
    };

    Group local_symbols;
    Group external_symbols;
    Group undefined_symbols;
};

namespace MachOPlatform {
enum {
    MACOS = 1,
    IOS = 2,
};
}

struct MachOVersion {
    std::uint16_t major;
    std::uint8_t minor;
    std::uint8_t patch;
};

struct MachOBuildVersionCommand {
    std::uint32_t platform;
    MachOVersion os_version;
    MachOVersion sdk_version;
};

namespace MachOCPUType {
enum {
    ARM64 = 0x0100000C,
};
}

namespace MachOCPUSubType {
enum {
    ARM64_ALL = 0x0,
};
}

typedef std::variant<MachOSegment, MachOSymtabCommand, MachODysymtabCommand, MachOBuildVersionCommand> MachOCommand;

struct MachOFile {
    std::uint32_t cpu_type;
    std::uint32_t cpu_sub_type;
    std::vector<MachOCommand> load_commands;
};

} // namespace banjo

#endif
