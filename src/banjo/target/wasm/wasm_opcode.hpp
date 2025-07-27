#ifndef BANJO_TARGET_WASM_OPCODE_H
#define BANJO_TARGET_WASM_OPCODE_H

#include "banjo/mcode/instruction.hpp"

#include <string_view>
#include <unordered_map>

namespace banjo::target {

namespace WasmOpcode {
enum {
    END,
    LOCAL_GET,
    LOCAL_SET,
    I32_ADD,
    I32_SUB,
    I32_MUL,
    I32_DIV_S,
    I32_DIV_U,
    I32_REM_S,
    I32_REM_U,
    F32_ADD,
    F32_SUB,
    F32_MUL,
    F32_DIV,
};

extern const std::unordered_map<mcode::Opcode, std::string_view> NAMES;

} // namespace WasmOpcode

} // namespace banjo::target

#endif
