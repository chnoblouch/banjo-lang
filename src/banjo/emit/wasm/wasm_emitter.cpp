#include "wasm_emitter.hpp"

#include "banjo/emit/wasm/wasm_builder.hpp"
#include "banjo/emit/wasm/wasm_format.hpp"
#include "banjo/utils/macros.hpp"

#include <variant>

namespace banjo::codegen {

void WASMEmitter::generate() {
    WasmObjectFile file = WasmBuilder().build();

    emit_header();
    emit_type_section(file);
    emit_import_section(file);
    emit_function_section(file);
    emit_code_section(file);
    emit_linking_section(file);
}

void WASMEmitter::emit_header() {
    // magic number
    emit_u8('\0');
    emit_u8('a');
    emit_u8('s');
    emit_u8('m');

    // version
    emit_u8(0x01);
    emit_u8(0x00);
    emit_u8(0x00);
    emit_u8(0x00);
}

void WASMEmitter::emit_type_section(const WasmObjectFile &file) {
    WriteBuffer data;
    write_uleb128(data, file.types.size()); // number of types

    for (const WasmFunctionType &type : file.types) {
        data.write_u8(0x60); // leading byte for function types

        write_uleb128(data, type.param_types.size()); // number of parameter types

        for (std::uint8_t param_type : type.param_types) {
            data.write_u8(param_type); // parameter type
        }

        write_uleb128(data, type.result_types.size()); // number of result types

        for (std::uint8_t result_type : type.result_types) {
            data.write_u8(result_type); // result type
        }
    }

    emit_section(1, data);
}

void WASMEmitter::emit_import_section(const WasmObjectFile &file) {
    WriteBuffer data;
    write_uleb128(data, file.imports.size()); // number of imports

    for (const WasmImport &import : file.imports) {
        write_name(data, import.mod);  // import module
        write_name(data, import.name); // import name

        if (const auto *memory = std::get_if<WasmMemory>(&import.type)) {
            data.write_u8(0x02);                   // import type (memory)
            data.write_u8(0x00);                   // indicate that no maximum size is present
            write_uleb128(data, memory->min_size); // minimum memory size
        }
    }

    emit_section(2, data);
}

void WASMEmitter::emit_function_section(const WasmObjectFile &file) {
    WriteBuffer data;
    write_uleb128(data, file.functions.size()); // number of functions

    for (const WasmFunction &function : file.functions) {
        write_uleb128(data, function.type_index); // type index
    }

    emit_section(3, data);
}

void WASMEmitter::emit_code_section(const WasmObjectFile &file) {
    WriteBuffer data;
    write_uleb128(data, file.functions.size()); // number of functions

    for (const WasmFunction &function : file.functions) {
        WriteBuffer buffer;
        buffer.write_u8(0x00);                                         // number of locals
        buffer.write_data(function.body.data(), function.body.size()); // body

        write_uleb128(data, buffer.get_size()); // function code size
        data.write_data(buffer);                // function code
    }

    emit_section(10, data);
}

void WASMEmitter::emit_linking_section(const WasmObjectFile &file) {
    WriteBuffer data;
    write_name(data, "linking"); // custom section name
    data.write_u8(0x02);         // linking metadata version (2)

    write_symbol_table_subsection(data, file.symbols);

    emit_section(0, data);
}

void WASMEmitter::write_symbol_table_subsection(WriteBuffer &buffer, const std::vector<WasmSymbol> &symbols) {
    WriteBuffer data;
    write_uleb128(data, symbols.size()); // number of symbols

    for (const WasmSymbol &symbol : symbols) {
        data.write_u8(symbol.type);        // symbol type
        write_uleb128(data, symbol.flags); // symbol flags
        write_uleb128(data, symbol.index); // object index
        write_name(data, symbol.name);     // symbol name
    }

    buffer.write_u8(0x08);                  // section type
    write_uleb128(buffer, data.get_size()); // section size
    buffer.write_data(data);                // section payload
}

void WASMEmitter::emit_section(std::uint8_t id, const WriteBuffer &data) {
    emit_u8(id);                   // id
    emit_uleb128(data.get_size()); // size
    emit_data(data);               // payload
}

void WASMEmitter::emit_uleb128(std::uint64_t value) {
    LEB128Buffer encoding = encode_uleb128(value);
    emit_data(encoding.get_data(), encoding.get_size());
}

void WASMEmitter::write_uleb128(WriteBuffer &buffer, std::uint64_t value) {
    LEB128Buffer encoding = encode_uleb128(value);
    buffer.write_data(encoding.get_data(), encoding.get_size());
}

void WASMEmitter::write_name(WriteBuffer &buffer, const std::string &name) {
    buffer.write_u8(static_cast<std::uint8_t>(name.size()));
    buffer.write_data(name.data(), name.size());
}

WASMEmitter::LEB128Buffer WASMEmitter::encode_uleb128(std::uint64_t value) {
    ASSERT(value < 128);

    LEB128Buffer buffer;
    buffer.append(value);
    return buffer;
}

} // namespace banjo::codegen
