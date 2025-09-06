#ifndef BANJO_EMIT_WASM_FORMAT_H
#define BANJO_EMIT_WASM_FORMAT_H

#include <cstdint>
#include <string>
#include <variant>
#include <vector>

namespace banjo {

namespace WasmValueType {
enum {
    I32 = 0x7F,
    I64 = 0x7E,
    F32 = 0x7D,
    F64 = 0x7C,
};
}

struct WasmFunctionType {
    std::vector<std::uint8_t> param_types;
    std::vector<std::uint8_t> result_types;
};

namespace WasmReferenceType {
enum {
    FUNCREF = 0x70,
};
}

struct WasmTypeIndex {
    std::uint32_t value;
};

struct WasmTable {
    std::uint8_t element_type;
    std::uint32_t min_size;
};

struct WasmMemory {
    std::uint32_t min_size;
};

struct WasmGlobalType {
    std::uint8_t type;
    bool mut;
};

struct WasmImport {
    std::string mod;
    std::string name;
    std::variant<WasmTypeIndex, WasmTable, WasmMemory, WasmGlobalType> kind;
};

struct WasmLocalGroup {
    unsigned count;
    std::uint8_t type;
};

namespace WasmRelocType {
enum {
    FUNCTION_INDEX_LEB = 0x00,
    TABLE_INDEX_SLEB = 0x01,
    MEMORY_ADDR_LEB = 0x03,
    MEMORY_ADDR_SLEB = 0x04,
    TYPE_INDEX_LEB = 0x06,
    GLOBAL_INDEX_LEB = 0x07,
    TABLE_NUMBER_LEB = 0x14,
};
}

struct WasmRelocation {
    std::uint8_t type;
    std::uint32_t offset;
    std::uint32_t index;
    std::uint32_t addend = 0;
};

struct WasmFunction {
    std::uint32_t type_index;
    std::vector<WasmLocalGroup> local_groups;
    std::vector<std::uint8_t> body;
    std::vector<WasmRelocation> relocs;
};

struct WasmDataSegment {
    std::vector<std::uint8_t> offset_expr;
    std::vector<std::uint8_t> init_bytes;
};

namespace WasmSymbolType {
enum {
    FUNCTION = 0x00,
    DATA = 0x01,
    GLOBAL = 0x02,
    TABLE = 0x05,
};
}

namespace WasmSymbolFlags {
enum {
    BINDING_LOCAL = 0x02,
    UNDEFINED = 0x10,
    EXPORTED = 0x20,
    NO_STRIP = 0x80,
};
};

struct WasmSymbol {
    std::uint8_t type;
    std::uint32_t flags;
    std::string name = "";

    std::uint32_t index = 0;
    std::uint32_t offset = 0;
    std::uint32_t size = 0;
};

struct WasmObjectFile {
    std::vector<WasmFunctionType> types;
    std::vector<WasmImport> imports;
    std::vector<WasmFunction> functions;
    std::vector<WasmDataSegment> data_segments;
    std::vector<WasmSymbol> symbols;
};

}; // namespace banjo

#endif
