#ifndef BANJO_EMIT_WASM_BUILDER_H
#define BANJO_EMIT_WASM_BUILDER_H

#include "banjo/emit/wasm/wasm_format.hpp"
#include "banjo/mcode/module.hpp"
#include "banjo/target/wasm/wasm_mcode.hpp"
#include "banjo/utils/write_buffer.hpp"

#include <cstdint>
#include <string_view>
#include <unordered_map>

namespace banjo {

class WasmBuilder {

private:
    unsigned num_func_imports = 0;
    std::uint32_t data_offset = 0;
    std::unordered_map<std::string_view, std::uint32_t> symbol_indices;

    struct FuncContext {
        mcode::Function &func;
        WriteBuffer body;
        std::vector<WasmRelocation> relocs;
    };

public:
    WasmObjectFile build(mcode::Module &mod);

private:
    void collect_symbol_indices(mcode::Module &mod);

    void build_func_import(WasmObjectFile &file, const target::WasmFuncImport &func_import);
    void build_extern_global(WasmObjectFile &file, const std::string &extern_global);
    void build_global(WasmObjectFile &file, mcode::Global &global);
    void build_func(WasmObjectFile &file, mcode::Function &func);

    void encode_instrs(FuncContext &ctx);
    void encode_instr(FuncContext &ctx, mcode::Instruction &instr);
    void encode_block(FuncContext &ctx);
    void encode_loop(FuncContext &ctx);
    void encode_br(FuncContext &ctx, mcode::Instruction &instr);
    void encode_br_if(FuncContext &ctx, mcode::Instruction &instr);
    void encode_br_table(FuncContext &ctx, mcode::Instruction &instr);
    void encode_call(FuncContext &ctx, mcode::Instruction &instr);
    void encode_local_get(FuncContext &ctx, mcode::Instruction &instr);
    void encode_local_set(FuncContext &ctx, mcode::Instruction &instr);
    void encode_local_tee(FuncContext &ctx, mcode::Instruction &instr);
    void encode_global_get(FuncContext &ctx, mcode::Instruction &instr);
    void encode_global_set(FuncContext &ctx, mcode::Instruction &instr);
    void encode_i32_load(FuncContext &ctx, mcode::Instruction &instr);
    void encode_i64_load(FuncContext &ctx, mcode::Instruction &instr);
    void encode_f32_load(FuncContext &ctx, mcode::Instruction &instr);
    void encode_f64_load(FuncContext &ctx, mcode::Instruction &instr);
    void encode_i32_store(FuncContext &ctx, mcode::Instruction &instr);
    void encode_i64_store(FuncContext &ctx, mcode::Instruction &instr);
    void encode_f32_store(FuncContext &ctx, mcode::Instruction &instr);
    void encode_f64_store(FuncContext &ctx, mcode::Instruction &instr);
    void encode_i32_store8(FuncContext &ctx, mcode::Instruction &instr);
    void encode_i32_store16(FuncContext &ctx, mcode::Instruction &instr);
    void encode_i32_const(FuncContext &ctx, mcode::Instruction &instr);

    void encode_load_store_addr(FuncContext &ctx, mcode::Operand &addr);

    WasmFunctionType build_func_type(const target::WasmFuncType &type);
    std::vector<WasmLocalGroup> build_local_groups(mcode::Function &func);
    std::uint8_t build_value_type(target::WasmType type);

    void write_reloc_placeholder_32(WriteBuffer &buffer);
};

} // namespace banjo

#endif
