#ifndef BANJO_EMIT_WASM_EMITTER_H
#define BANJO_EMIT_WASM_EMITTER_H

#include "banjo/emit/binary_emitter.hpp"
#include "banjo/emit/wasm/wasm_format.hpp"
#include "banjo/utils/fixed_vector.hpp"
#include "banjo/utils/write_buffer.hpp"

#include <cstdint>
#include <string>

namespace banjo::codegen {

class WasmEmitter final : public BinaryEmitter {

private:
    typedef FixedVector<unsigned char, 16> LEB128Buffer;

public:
    WasmEmitter(mcode::Module &module, std::ostream &stream) : BinaryEmitter(module, stream) {}
    void generate() override;

private:
    void emit_header();
    void emit_type_section(const WasmObjectFile &file);
    void emit_import_section(const WasmObjectFile &file);
    void emit_function_section(const WasmObjectFile &file);
    void emit_code_section(const WasmObjectFile &file);
    void emit_linking_section(const WasmObjectFile &file);

    void emit_section(std::uint8_t id, const WriteBuffer &data);
    void emit_uleb128(std::uint64_t value);

    void write_symbol_table_subsection(WriteBuffer &buffer, const std::vector<WasmSymbol> &symbols);

    void write_name(WriteBuffer &buffer, const std::string &name);
    void write_uleb128(WriteBuffer &buffer, std::uint64_t value);

    LEB128Buffer encode_uleb128(std::uint64_t value);
};

} // namespace banjo::codegen

#endif
