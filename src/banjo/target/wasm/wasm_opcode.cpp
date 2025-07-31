#include "wasm_opcode.hpp"
#include <string_view>

namespace banjo::target {

const std::unordered_map<mcode::Opcode, std::string_view> WasmOpcode::NAMES{
    {END, "end"},
    {CALL, "call"},
    {DROP, "drop"},
    {LOCAL_GET, "local.get"},
    {LOCAL_SET, "local.set"},
    {I32_CONST, "i32.const"},
    {I64_CONST, "i64.const"},
    {F32_CONST, "f32.const"},
    {F64_CONST, "f64.const"},
    {I32_ADD, "i32.add"},
    {I32_SUB, "i32.sub"},
    {I32_MUL, "i32.mul"},
    {I32_DIV_S, "i32.div_s"},
    {I32_DIV_U, "i32.div_u"},
    {I32_REM_S, "i32.rem_s"},
    {I32_REM_U, "i32.rem_u"},
    {I64_ADD, "i64.add"},
    {I64_SUB, "i64.sub"},
    {I64_MUL, "i64.mul"},
    {I64_DIV_S, "i64.div_s"},
    {I64_DIV_U, "i64.div_u"},
    {I64_REM_S, "i64.rem_s"},
    {I64_REM_U, "i64.rem_u"},
    {F32_ADD, "f32.add"},
    {F32_SUB, "f32.sub"},
    {F32_MUL, "f32.mul"},
    {F32_DIV, "f32.div"},
};

}
