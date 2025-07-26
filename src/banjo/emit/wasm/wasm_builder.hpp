#ifndef BANJO_EMIT_WASM_BUILDER_H
#define BANJO_EMIT_WASM_BUILDER_H

#include "banjo/emit/wasm/wasm_format.hpp"
#include "banjo/mcode/module.hpp"
#include "banjo/utils/write_buffer.hpp"

#include <cstdint>

namespace banjo {

class WasmBuilder {

public:
    WasmObjectFile build(mcode::Module &mod);

private:
    void build_func(WasmObjectFile &file, mcode::Function &func);
    std::vector<std::uint8_t> encode_instrs(mcode::Function &func);
    void encode_instr(WriteBuffer &buffer, mcode::Instruction &instr);

    WasmFunctionType build_func_type(mcode::Function &func);
    std::uint8_t build_value_type(ssa::Type type);
};

} // namespace banjo

#endif
