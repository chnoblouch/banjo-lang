#ifndef BANJO_TARGET_WASM_OPCODE_H
#define BANJO_TARGET_WASM_OPCODE_H

#include "banjo/mcode/instruction.hpp"

#include <string_view>
#include <unordered_map>

namespace banjo::target {

namespace WasmOpcode {
enum {
    END,
    CALL,
    DROP,
    LOCAL_GET,
    LOCAL_SET,
    LOCAL_TEE,
    GLOBAL_GET,
    GLOBAL_SET,
    I32_LOAD,
    I64_LOAD,
    F32_LOAD,
    F64_LOAD,
    I32_STORE,
    I64_STORE,
    F32_STORE,
    F64_STORE,
    I32_CONST,
    I64_CONST,
    F32_CONST,
    F64_CONST,
    I32_ADD,
    I32_SUB,
    I32_MUL,
    I32_DIV_S,
    I32_DIV_U,
    I32_REM_S,
    I32_REM_U,
    I64_ADD,
    I64_SUB,
    I64_MUL,
    I64_DIV_S,
    I64_DIV_U,
    I64_REM_S,
    I64_REM_U,
    F32_ADD,
    F32_SUB,
    F32_MUL,
    F32_DIV,
};

extern const std::unordered_map<mcode::Opcode, std::string_view> NAMES;

} // namespace WasmOpcode

} // namespace banjo::target

#endif
