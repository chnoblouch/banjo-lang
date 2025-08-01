#include "wasm_builder.hpp"

#include "banjo/emit/wasm/wasm_format.hpp"
#include "banjo/mcode/function.hpp"
#include "banjo/target/wasm/wasm_mcode.hpp"
#include "banjo/target/wasm/wasm_opcode.hpp"
#include "banjo/utils/macros.hpp"

#include <any>

namespace banjo {

WasmObjectFile WasmBuilder::build(mcode::Module &mod) {
    const target::WasmModData &mod_data = std::any_cast<const target::WasmModData &>(mod.get_target_data());

    WasmObjectFile file{
        .imports{
            WasmImport{
                .mod = "env",
                .name = "__linear_memory",
                .kind{
                    WasmMemory{
                        .min_size = 0,
                    },
                }
            },
        },
    };

    collect_symbol_indices(mod);

    for (const target::WasmFuncImport &func_import : mod_data.func_imports) {
        build_func_import(file, func_import);
    }

    for (mcode::Global &global : mod.get_globals()) {
        build_global(file, global);
    }

    for (mcode::Function *func : mod.get_functions()) {
        build_func(file, *func);
    }

    return file;
}

void WasmBuilder::collect_symbol_indices(mcode::Module &mod) {
    const target::WasmModData &mod_data = std::any_cast<const target::WasmModData &>(mod.get_target_data());

    unsigned index = 0;

    for (const target::WasmFuncImport &func_import : mod_data.func_imports) {
        symbol_indices.insert({func_import.name, index});
        index += 1;
    }

    for (const mcode::Global &global : mod.get_globals()) {
        if (std::holds_alternative<std::string>(global.value)) {
            symbol_indices.insert({global.name, index});
            index += 1;
        }
    }

    for (mcode::Function *func : mod.get_functions()) {
        symbol_indices.insert({func->get_name(), index});
        index += 1;
    }
}

void WasmBuilder::build_func_import(WasmObjectFile &file, const target::WasmFuncImport &func_import) {
    file.types.push_back(build_func_type(func_import.type));

    file.imports.push_back(
        WasmImport{
            .mod = func_import.mod,
            .name = func_import.name,
            .kind{
                WasmTypeIndex{
                    .value = static_cast<std::uint32_t>(file.types.size() - 1),
                },
            },
        }
    );

    file.symbols.push_back(
        WasmSymbol{
            .type = WasmSymbolType::FUNCTION,
            .flags = WasmSymbolFlags::UNDEFINED,
            .index = static_cast<std::uint32_t>(num_func_imports),
        }
    );

    num_func_imports += 1;
}

void WasmBuilder::build_global(WasmObjectFile &file, mcode::Global &global) {
    if (auto string = std::get_if<std::string>(&global.value)) {
        WriteBuffer offset_buffer;
        offset_buffer.write_u8(0x41);
        offset_buffer.write_sleb128(data_offset);
        offset_buffer.write_u8(0x0B);

        WriteBuffer init_bytes;
        init_bytes.write_data(string->data(), string->size());

        file.data_segments.push_back(
            WasmDataSegment{
                .offset_expr = offset_buffer.move_data(),
                .init_bytes = init_bytes.move_data(),
            }
        );

        file.symbols.push_back(
            WasmSymbol{
                .type = WasmSymbolType::DATA,
                .flags = WasmSymbolFlags::BINDING_LOCAL,
                .name = global.name,
                .index = static_cast<std::uint32_t>(file.data_segments.size() - 1),
                .offset = 0,
                .size = static_cast<std::uint32_t>(string->size()),
            }
        );

        data_offset += string->size();
    }
}

void WasmBuilder::build_func(WasmObjectFile &file, mcode::Function &func) {
    const target::WasmFuncData &func_data = std::any_cast<const target::WasmFuncData &>(func.get_target_data());

    file.types.push_back(build_func_type(func_data.type));

    std::vector<WasmRelocation> relocs;
    std::vector<std::uint8_t> body = encode_instrs(func, relocs);

    file.functions.push_back(
        WasmFunction{
            .type_index = static_cast<std::uint32_t>(file.types.size() - 1),
            .local_groups = build_local_groups(func),
            .body = std::move(body),
            .relocs = std::move(relocs),
        }
    );

    file.symbols.push_back(
        WasmSymbol{
            .type = WasmSymbolType::FUNCTION,
            .flags = WasmSymbolFlags::EXPORTED,
            .name = func.get_name(),
            .index = static_cast<std::uint32_t>(num_func_imports + file.functions.size() - 1),
        }
    );
}

std::vector<std::uint8_t> WasmBuilder::encode_instrs(mcode::Function &func, std::vector<WasmRelocation> &relocs) {
    WriteBuffer buffer;

    for (mcode::BasicBlock &block : func) {
        for (mcode::Instruction &instr : block) {
            encode_instr(buffer, instr, relocs);
        }
    }

    return buffer.move_data();
}

void WasmBuilder::encode_instr(WriteBuffer &buffer, mcode::Instruction &instr, std::vector<WasmRelocation> &relocs) {
    switch (instr.get_opcode()) {
        case target::WasmOpcode::END: buffer.write_u8(0x0B); break;
        case target::WasmOpcode::CALL: encode_call(buffer, instr, relocs); break;
        case target::WasmOpcode::DROP: buffer.write_u8(0x1A); break;
        case target::WasmOpcode::LOCAL_GET:
            buffer.write_u8(0x20);
            buffer.write_uleb128(instr.get_operand(0).get_int_immediate().to_u64());
            break;
        case target::WasmOpcode::LOCAL_SET:
            buffer.write_u8(0x21);
            buffer.write_uleb128(instr.get_operand(0).get_int_immediate().to_u64());
            break;
        case target::WasmOpcode::I32_CONST: encode_i32_const(buffer, instr, relocs); break;
        case target::WasmOpcode::I64_CONST:
            buffer.write_u8(0x42);
            buffer.write_sleb128(instr.get_operand(0).get_int_immediate());
            break;
        case target::WasmOpcode::F32_CONST:
            buffer.write_u8(0x43);
            buffer.write_f32(instr.get_operand(0).get_fp_immediate());
            break;
        case target::WasmOpcode::F64_CONST:
            buffer.write_u8(0x44);
            buffer.write_f64(instr.get_operand(0).get_fp_immediate());
            break;
        case target::WasmOpcode::I32_ADD: buffer.write_u8(0x6A); break;
        case target::WasmOpcode::I32_SUB: buffer.write_u8(0x6B); break;
        case target::WasmOpcode::I32_MUL: buffer.write_u8(0x6C); break;
        case target::WasmOpcode::I32_DIV_S: buffer.write_u8(0x6D); break;
        case target::WasmOpcode::I32_DIV_U: buffer.write_u8(0x6E); break;
        case target::WasmOpcode::I32_REM_S: buffer.write_u8(0x6F); break;
        case target::WasmOpcode::I32_REM_U: buffer.write_u8(0x70); break;
        case target::WasmOpcode::I64_ADD: buffer.write_u8(0x7C); break;
        case target::WasmOpcode::I64_SUB: buffer.write_u8(0x7D); break;
        case target::WasmOpcode::I64_MUL: buffer.write_u8(0x7E); break;
        case target::WasmOpcode::I64_DIV_S: buffer.write_u8(0x7F); break;
        case target::WasmOpcode::I64_DIV_U: buffer.write_u8(0x80); break;
        case target::WasmOpcode::I64_REM_S: buffer.write_u8(0x81); break;
        case target::WasmOpcode::I64_REM_U: buffer.write_u8(0x82); break;
        case target::WasmOpcode::F32_ADD: buffer.write_u8(0x92); break;
        default: ASSERT_UNREACHABLE;
    }
}

void WasmBuilder::encode_call(WriteBuffer &buffer, mcode::Instruction &instr, std::vector<WasmRelocation> &relocs) {
    mcode::Operand &callee = instr.get_operand(0);

    buffer.write_u8(0x10);

    relocs.push_back(
        WasmRelocation{
            .type = WasmRelocType::FUNCTION_INDEX_LEB,
            .offset = static_cast<std::uint32_t>(buffer.get_size()),
            .index = symbol_indices.at(callee.get_symbol().name),
        }
    );

    write_reloc_placeholder_32(buffer);
}

void WasmBuilder::encode_i32_const(
    WriteBuffer &buffer,
    mcode::Instruction &instr,
    std::vector<WasmRelocation> &relocs
) {
    mcode::Operand &operand = instr.get_operand(0);

    buffer.write_u8(0x41);

    if (operand.is_int_immediate()) {
        buffer.write_sleb128(operand.get_int_immediate());
    } else if (operand.is_symbol()) {
        relocs.push_back(
            WasmRelocation{
                .type = WasmRelocType::MEMORY_ADDR_SLEB,
                .offset = static_cast<std::uint32_t>(buffer.get_size()),
                .index = symbol_indices.at(operand.get_symbol().name),
                .addend = 0,
            }
        );

        write_reloc_placeholder_32(buffer);
    }
}

WasmFunctionType WasmBuilder::build_func_type(const target::WasmFuncType &type) {
    std::vector<std::uint8_t> param_types(type.params.size());

    for (unsigned i = 0; i < type.params.size(); i++) {
        param_types[i] = build_value_type(type.params[i]);
    }

    std::vector<std::uint8_t> result_types;

    if (type.result_type) {
        result_types.push_back(build_value_type(*type.result_type));
    }

    return WasmFunctionType{
        .param_types = param_types,
        .result_types = result_types,
    };
}

std::vector<WasmLocalGroup> WasmBuilder::build_local_groups(mcode::Function &func) {
    const target::WasmFuncData &func_data = std::any_cast<const target::WasmFuncData &>(func.get_target_data());

    std::vector<WasmLocalGroup> local_groups;
    std::optional<target::WasmType> last_type;

    for (target::WasmType type : func_data.locals) {
        if (type == last_type) {
            local_groups.back().count += 1;
        } else {
            local_groups.push_back(WasmLocalGroup{.count = 1, .type = build_value_type(type)});
            last_type = type;
        }
    }

    return local_groups;
}

std::uint8_t WasmBuilder::build_value_type(target::WasmType type) {
    switch (type) {
        case target::WasmType::I32: return WasmValueType::I32;
        case target::WasmType::I64: return WasmValueType::I64;
        case target::WasmType::F32: return WasmValueType::F32;
        case target::WasmType::F64: return WasmValueType::F64;
    }
}

void WasmBuilder::write_reloc_placeholder_32(WriteBuffer &buffer) {
    buffer.write_u8(0x80);
    buffer.write_u8(0x80);
    buffer.write_u8(0x80);
    buffer.write_u8(0x80);
    buffer.write_u8(0x00);
}

} // namespace banjo
