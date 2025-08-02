#ifndef BANJO_TARGET_WASM_MCODE_H
#define BANJO_TARGET_WASM_MCODE_H

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace banjo::target {

enum class WasmType : std::uint8_t {
    I32,
    I64,
    F32,
    F64,
};

struct WasmFuncType {
    std::vector<WasmType> params;
    std::optional<WasmType> result_type;
};

struct WasmFuncData {
    WasmFuncType type;
    std::vector<WasmType> locals;
    unsigned stack_pointer_local;
};

struct WasmFuncImport {
    std::string mod;
    std::string name;
    WasmFuncType type;
};

struct WasmModData {
    std::vector<WasmFuncImport> func_imports;
};

} // namespace banjo::target

#endif
