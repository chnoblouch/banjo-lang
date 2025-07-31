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

struct WasmTypeIndex {
    std::uint32_t value;
};

struct WasmMemory {
    std::uint32_t min_size;
};

struct WasmImport {
    std::string mod;
    std::string name;
    std::variant<WasmTypeIndex, WasmMemory> kind;
};

struct WasmLocalGroup {
    unsigned count;
    std::uint8_t type;
};

namespace WasmRelocType {
enum {
    FUNCTION_INDEX_LEB = 0x00,
};
}

struct WasmRelocation {
    std::uint8_t type;
    std::uint32_t offset;
    std::uint32_t index;
};

struct WasmFunction {
    std::uint32_t type_index;
    std::vector<WasmLocalGroup> local_groups;
    std::vector<std::uint8_t> body;
    std::vector<WasmRelocation> relocs;
};

namespace WasmSymbolType {
enum {
    FUNCTION = 0,
};
}

namespace WasmSymbolFlags {
enum {
    UNDEFINED = 0x10,
    EXPORTED = 0x20,
};
};

struct WasmSymbol {
    std::uint8_t type;
    std::uint32_t flags;
    std::uint32_t index;
    std::string name;
};

struct WasmObjectFile {
    std::vector<WasmFunctionType> types;
    std::vector<WasmImport> imports;
    std::vector<WasmFunction> functions;
    std::vector<WasmSymbol> symbols;
};

}; // namespace banjo

#endif
