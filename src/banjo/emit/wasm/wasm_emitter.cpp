#include "wasm_emitter.hpp"

#include "banjo/emit/wasm/wasm_builder.hpp"
#include "banjo/emit/wasm/wasm_format.hpp"
#include "banjo/utils/utils.hpp"

#include <variant>

namespace banjo::codegen {

void WasmEmitter::generate() {
    WasmObjectFile file = WasmBuilder().build(module);

    code_reloc_section = RelocSection{
        .name = "reloc.CODE",
        .section_index = 4,
        .relocs{},
    };

    emit_header();
    emit_type_section(file);
    emit_import_section(file);
    emit_function_section(file);
    emit_data_count_section(file);
    emit_code_section(file);
    emit_data_section(file);
    emit_linking_section(file);

    if (!code_reloc_section.relocs.empty()) {
        emit_reloc_section(code_reloc_section);
    }
}

void WasmEmitter::emit_header() {
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

void WasmEmitter::emit_type_section(const WasmObjectFile &file) {
    WriteBuffer data;
    data.write_uleb128(file.types.size()); // number of types

    for (const WasmFunctionType &type : file.types) {
        data.write_u8(0x60); // leading byte for function types

        data.write_uleb128(type.param_types.size()); // number of parameter types

        for (std::uint8_t param_type : type.param_types) {
            data.write_u8(param_type); // parameter type
        }

        data.write_uleb128(type.result_types.size()); // number of result types

        for (std::uint8_t result_type : type.result_types) {
            data.write_u8(result_type); // result type
        }
    }

    emit_section(1, data);
}

void WasmEmitter::emit_import_section(const WasmObjectFile &file) {
    WriteBuffer data;
    data.write_uleb128(file.imports.size()); // number of imports

    for (const WasmImport &import : file.imports) {
        write_name(data, import.mod);  // import module
        write_name(data, import.name); // import name

        if (auto type_index = std::get_if<WasmTypeIndex>(&import.kind)) {
            data.write_u8(0x00);                   // import type (type index)
            data.write_uleb128(type_index->value); // function type index
        } else if (auto table = std::get_if<WasmTable>(&import.kind)) {
            data.write_u8(0x01);                 // import type (table)
            data.write_u8(table->element_type);  // element reference type
            data.write_u8(0x00);                 // indicate that no maximum size is present
            data.write_uleb128(table->min_size); // minimum memory size
        } else if (auto memory = std::get_if<WasmMemory>(&import.kind)) {
            data.write_u8(0x02);                  // import type (memory)
            data.write_u8(0x00);                  // indicate that no maximum size is present
            data.write_uleb128(memory->min_size); // minimum memory size
        } else if (auto global_type = std::get_if<WasmGlobalType>(&import.kind)) {
            data.write_u8(0x03);                           // import type (global)
            data.write_u8(global_type->type);              // value type
            data.write_u8(global_type->mut ? 0x01 : 0x00); // mutability
        }
    }

    emit_section(2, data);
}

void WasmEmitter::emit_function_section(const WasmObjectFile &file) {
    WriteBuffer data;
    data.write_uleb128(file.functions.size()); // number of functions

    for (const WasmFunction &function : file.functions) {
        data.write_uleb128(function.type_index); // type index
    }

    emit_section(3, data);
}

void WasmEmitter::emit_code_section(const WasmObjectFile &file) {
    WriteBuffer data;
    data.write_uleb128(file.functions.size()); // number of functions

    for (const WasmFunction &function : file.functions) {
        WriteBuffer buffer;
        buffer.write_uleb128(function.local_groups.size()); // number of local groups

        for (const WasmLocalGroup &local_group : function.local_groups) {
            buffer.write_uleb128(local_group.count); // number of locals
            buffer.write_u8(local_group.type);       // value type
        }

        buffer.write_data(function.body.data(), function.body.size()); // body

        data.write_uleb128(buffer.get_size()); // function code size
        std::uint32_t relocs_base_offset = data.get_size() + buffer.get_size() - function.body.size();
        data.write_data(buffer); // function code

        for (const WasmRelocation &reloc : function.relocs) {
            code_reloc_section.relocs.push_back(
                WasmRelocation{
                    .type = reloc.type,
                    .offset = static_cast<std::uint32_t>(relocs_base_offset + reloc.offset),
                    .index = reloc.index,
                }
            );
        }
    }

    emit_section(10, data);
}

void WasmEmitter::emit_data_section(const WasmObjectFile &file) {
    WriteBuffer data;
    data.write_uleb128(file.data_segments.size()); // number of data segments

    for (const WasmDataSegment &segment : file.data_segments) {
        data.write_u8(0x00); // kind (active data segment in memory 0)
        data.write_data(segment.offset_expr.data(), segment.offset_expr.size()); // offset expression
        data.write_uleb128(segment.init_bytes.size());                           // initial value size
        data.write_data(segment.init_bytes.data(), segment.init_bytes.size());   // initial value
    }

    emit_section(11, data);
}

void WasmEmitter::emit_data_count_section(const WasmObjectFile &file) {
    WriteBuffer data;
    data.write_uleb128(file.data_segments.size()); // number of data segments
    emit_section(12, data);
}

void WasmEmitter::emit_reloc_section(const RelocSection &section) {
    WriteBuffer data;
    write_name(data, section.name);            // custom section name
    data.write_uleb128(section.section_index); // target section index
    data.write_uleb128(section.relocs.size()); // number of relocations

    for (const WasmRelocation &reloc : section.relocs) {
        data.write_u8(reloc.type);        // relocation type
        data.write_uleb128(reloc.offset); // relocation offset
        data.write_uleb128(reloc.index);  // symbol index

        if (reloc.type == WasmRelocType::MEMORY_ADDR_LEB || reloc.type == WasmRelocType::MEMORY_ADDR_SLEB) {
            data.write_uleb128(reloc.addend); // address addend
        }
    }

    emit_section(0, data);
}

void WasmEmitter::emit_linking_section(const WasmObjectFile &file) {
    WriteBuffer data;
    write_name(data, "linking"); // custom section name
    data.write_u8(0x02);         // linking metadata version (2)

    write_symbol_table_subsection(data, file.symbols);

    emit_section(0, data);
}

void WasmEmitter::write_symbol_table_subsection(WriteBuffer &buffer, const std::vector<WasmSymbol> &symbols) {
    WriteBuffer data;
    data.write_uleb128(symbols.size()); // number of symbols

    for (const WasmSymbol &symbol : symbols) {
        data.write_u8(symbol.type);       // symbol type
        data.write_uleb128(symbol.flags); // symbol flags

        if (Utils::is_one_of(symbol.type, {WasmSymbolType::FUNCTION, WasmSymbolType::GLOBAL, WasmSymbolType::TABLE})) {
            data.write_uleb128(symbol.index); // element index

            if (!(symbol.flags & WasmSymbolFlags::UNDEFINED)) {
                write_name(data, symbol.name); // symbol name
            }
        } else if (symbol.type == WasmSymbolType::DATA) {
            write_name(data, symbol.name); // symbol name

            if (!(symbol.flags & WasmSymbolFlags::UNDEFINED)) {
                data.write_uleb128(symbol.index);  // data segment index
                data.write_uleb128(symbol.offset); // offset within the segment
                data.write_uleb128(symbol.size);   // data size
            }
        }
    }

    buffer.write_u8(0x08);                 // section type
    buffer.write_uleb128(data.get_size()); // section size
    buffer.write_data(data);               // section payload
}

void WasmEmitter::emit_section(std::uint8_t id, const WriteBuffer &data) {
    emit_u8(id);                   // id
    emit_uleb128(data.get_size()); // size
    emit_data(data);               // payload
}

void WasmEmitter::emit_uleb128(std::uint64_t value) {
    Utils::LEB128Buffer encoding = Utils::encode_uleb128(value);
    emit_data(encoding.get_data(), encoding.get_size());
}

void WasmEmitter::write_name(WriteBuffer &buffer, const std::string &name) {
    buffer.write_uleb128(name.size());
    buffer.write_data(name.data(), name.size());
}

} // namespace banjo::codegen
