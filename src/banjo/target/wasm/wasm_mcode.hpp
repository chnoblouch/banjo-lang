#ifndef BANJO_TARGET_WASM_MCODE_H
#define BANJO_TARGET_WASM_MCODE_H

#include <cstdint>
#include <optional>
#include <vector>

namespace banjo::target {

enum class WasmType : std::uint8_t {
    I32,
    I64,
    F32,
    F64,
};

struct WasmFunctionData {
    std::vector<WasmType> params;
    std::optional<WasmType> result_type;
    std::vector<WasmType> locals;
};

} // namespace banjo::target

#endif
