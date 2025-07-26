#include "wasm_opcode.hpp"
#include <string_view>

namespace banjo::target {

const std::unordered_map<mcode::Opcode, std::string_view> WasmOpcode::NAMES{
    {END, "end"},
    {LOCAL_GET, "local.get"},
    {I32_ADD, "i32.add"},
    {I32_SUB, "i32.sub"},
    {I32_MUL, "i32.mul"},
    {I32_DIV_S, "i32.div_s"},
    {I32_DIV_U, "i32.div_u"},
    {I32_REM_S, "i32.rem_s"},
    {I32_REM_U, "i32.rem_u"},
    {F32_ADD, "f32.add"},
    {F32_SUB, "f32.sub"},
    {F32_MUL, "f32.mul"},
    {F32_DIV, "f32.div"},
};

}
