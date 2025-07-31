#ifndef BANJO_EMIT_WASM_BUILDER_H
#define BANJO_EMIT_WASM_BUILDER_H

#include "banjo/emit/wasm/wasm_format.hpp"
#include "banjo/mcode/module.hpp"
#include "banjo/target/wasm/wasm_mcode.hpp"
#include "banjo/utils/write_buffer.hpp"

#include <cstdint>

namespace banjo {

class WasmBuilder {

private:
    unsigned num_func_imports = 0;

public:
    WasmObjectFile build(mcode::Module &mod);

private:
    void build_func(WasmObjectFile &file, mcode::Function &func);
    std::vector<std::uint8_t> encode_instrs(mcode::Function &func, std::vector<WasmRelocation> &relocs);
    void encode_instr(WriteBuffer &buffer, mcode::Instruction &instr, std::vector<WasmRelocation> &relocs);

    WasmFunctionType build_func_type(const target::WasmFuncType &type);
    std::vector<WasmLocalGroup> build_local_groups(mcode::Function &func);
    std::uint8_t build_value_type(target::WasmType type);
};

} // namespace banjo

#endif
