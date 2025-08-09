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
            WasmImport{
                .mod = "env",
                .name = "__stack_pointer",
                .kind{
                    WasmGlobalType{
                        .type = WasmValueType::I32,
                        .mut = true,
                    },
                },
            },
        },
        .symbols{
            WasmSymbol{
                .type = WasmSymbolType::GLOBAL,
                .flags = WasmSymbolFlags::UNDEFINED,
                .name = "__stack_pointer",
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

    symbol_indices.insert({"__stack_pointer", index});
    index += 1;

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

    FuncContext ctx{
        .func = func,
        .body{},
        .relocs{},
    };

    encode_instrs(ctx);

    file.functions.push_back(
        WasmFunction{
            .type_index = static_cast<std::uint32_t>(file.types.size() - 1),
            .local_groups = build_local_groups(func),
            .body = ctx.body.move_data(),
            .relocs = std::move(ctx.relocs),
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

void WasmBuilder::encode_instrs(FuncContext &ctx) {
    for (mcode::BasicBlock &block : ctx.func) {
        for (mcode::Instruction &instr : block) {
            encode_instr(ctx, instr);
        }
    }
}

void WasmBuilder::encode_instr(FuncContext &ctx, mcode::Instruction &instr) {
    switch (instr.get_opcode()) {
        case target::WasmOpcode::BLOCK: encode_block(ctx); break;
        case target::WasmOpcode::END_BLOCK: ctx.body.write_u8(0x0B); break;
        case target::WasmOpcode::LOOP: encode_loop(ctx); break;
        case target::WasmOpcode::END_LOOP: ctx.body.write_u8(0x0B); break;
        case target::WasmOpcode::BR: encode_br(ctx, instr); break;
        case target::WasmOpcode::BR_IF: encode_br_if(ctx, instr); break;
        case target::WasmOpcode::BR_TABLE: encode_br_table(ctx, instr); break;
        case target::WasmOpcode::END_FUNCTION: ctx.body.write_u8(0x0B); break;
        case target::WasmOpcode::CALL: encode_call(ctx, instr); break;
        case target::WasmOpcode::DROP: ctx.body.write_u8(0x1A); break;
        case target::WasmOpcode::LOCAL_GET: encode_local_get(ctx, instr); break;
        case target::WasmOpcode::LOCAL_SET: encode_local_set(ctx, instr); break;
        case target::WasmOpcode::LOCAL_TEE: encode_local_tee(ctx, instr); break;
        case target::WasmOpcode::GLOBAL_GET: encode_global_get(ctx, instr); break;
        case target::WasmOpcode::GLOBAL_SET: encode_global_set(ctx, instr); break;
        case target::WasmOpcode::I32_LOAD: encode_i32_load(ctx, instr); break;
        case target::WasmOpcode::I64_LOAD: encode_i64_load(ctx, instr); break;
        case target::WasmOpcode::F32_LOAD: encode_f32_load(ctx, instr); break;
        case target::WasmOpcode::F64_LOAD: encode_f64_load(ctx, instr); break;
        case target::WasmOpcode::I32_STORE: encode_i32_store(ctx, instr); break;
        case target::WasmOpcode::I64_STORE: encode_i64_store(ctx, instr); break;
        case target::WasmOpcode::F32_STORE: encode_f32_store(ctx, instr); break;
        case target::WasmOpcode::F64_STORE: encode_f64_store(ctx, instr); break;
        case target::WasmOpcode::I32_STORE8: encode_i32_store8(ctx, instr); break;
        case target::WasmOpcode::I32_STORE16: encode_i32_store16(ctx, instr); break;
        case target::WasmOpcode::I32_CONST: encode_i32_const(ctx, instr); break;
        case target::WasmOpcode::I64_CONST:
            ctx.body.write_u8(0x42);
            ctx.body.write_sleb128(instr.get_operand(0).get_int_immediate());
            break;
        case target::WasmOpcode::F32_CONST:
            ctx.body.write_u8(0x43);
            ctx.body.write_f32(instr.get_operand(0).get_fp_immediate());
            break;
        case target::WasmOpcode::F64_CONST:
            ctx.body.write_u8(0x44);
            ctx.body.write_f64(instr.get_operand(0).get_fp_immediate());
            break;
        case target::WasmOpcode::I32_ADD: ctx.body.write_u8(0x6A); break;
        case target::WasmOpcode::I32_SUB: ctx.body.write_u8(0x6B); break;
        case target::WasmOpcode::I32_MUL: ctx.body.write_u8(0x6C); break;
        case target::WasmOpcode::I32_DIV_S: ctx.body.write_u8(0x6D); break;
        case target::WasmOpcode::I32_DIV_U: ctx.body.write_u8(0x6E); break;
        case target::WasmOpcode::I32_REM_S: ctx.body.write_u8(0x6F); break;
        case target::WasmOpcode::I32_REM_U: ctx.body.write_u8(0x70); break;
        case target::WasmOpcode::I64_ADD: ctx.body.write_u8(0x7C); break;
        case target::WasmOpcode::I64_SUB: ctx.body.write_u8(0x7D); break;
        case target::WasmOpcode::I64_MUL: ctx.body.write_u8(0x7E); break;
        case target::WasmOpcode::I64_DIV_S: ctx.body.write_u8(0x7F); break;
        case target::WasmOpcode::I64_DIV_U: ctx.body.write_u8(0x80); break;
        case target::WasmOpcode::I64_REM_S: ctx.body.write_u8(0x81); break;
        case target::WasmOpcode::I64_REM_U: ctx.body.write_u8(0x82); break;
        case target::WasmOpcode::F32_ADD: ctx.body.write_u8(0x92); break;
        default: ASSERT_UNREACHABLE;
    }
}

void WasmBuilder::encode_block(FuncContext &ctx) {
    ctx.body.write_u8(0x02);
    ctx.body.write_u8(0x40);
}

void WasmBuilder::encode_loop(FuncContext &ctx) {
    ctx.body.write_u8(0x03);
    ctx.body.write_u8(0x40);
}

void WasmBuilder::encode_br(FuncContext &ctx, mcode::Instruction &instr) {
    unsigned label_index = instr.get_operand(0).get_int_immediate().to_u64();

    ctx.body.write_u8(0x0C);
    ctx.body.write_uleb128(label_index);
}

void WasmBuilder::encode_br_if(FuncContext &ctx, mcode::Instruction &instr) {
    unsigned label_index = instr.get_operand(0).get_int_immediate().to_u64();

    ctx.body.write_u8(0x0D);
    ctx.body.write_uleb128(label_index);
}

void WasmBuilder::encode_br_table(FuncContext &ctx, mcode::Instruction &instr) {
    ctx.body.write_u8(0x0E);
    ctx.body.write_uleb128(instr.get_operands().get_size() - 1);

    for (unsigned i = 0; i < instr.get_operands().get_size() - 1; i++) {
        ctx.body.write_uleb128(instr.get_operand(i).get_int_immediate().to_u64());
    }

    ctx.body.write_uleb128(instr.get_operand(instr.get_operands().get_size() - 1).get_int_immediate().to_u64());
}

void WasmBuilder::encode_call(FuncContext &ctx, mcode::Instruction &instr) {
    mcode::Operand &callee = instr.get_operand(0);

    ctx.body.write_u8(0x10);

    ctx.relocs.push_back(
        WasmRelocation{
            .type = WasmRelocType::FUNCTION_INDEX_LEB,
            .offset = static_cast<std::uint32_t>(ctx.body.get_size()),
            .index = symbol_indices.at(callee.get_symbol().name),
        }
    );

    write_reloc_placeholder_32(ctx.body);
}

void WasmBuilder::encode_local_get(FuncContext &ctx, mcode::Instruction &instr) {
    ctx.body.write_u8(0x20);
    ctx.body.write_uleb128(instr.get_operand(0).get_int_immediate().to_u64());
}

void WasmBuilder::encode_local_set(FuncContext &ctx, mcode::Instruction &instr) {
    ctx.body.write_u8(0x21);
    ctx.body.write_uleb128(instr.get_operand(0).get_int_immediate().to_u64());
}

void WasmBuilder::encode_local_tee(FuncContext &ctx, mcode::Instruction &instr) {
    ctx.body.write_u8(0x22);
    ctx.body.write_uleb128(instr.get_operand(0).get_int_immediate().to_u64());
}

void WasmBuilder::encode_global_get(FuncContext &ctx, mcode::Instruction &instr) {
    mcode::Operand &operand = instr.get_operand(0);

    ctx.body.write_u8(0x23);

    if (operand.is_int_immediate()) {
        ctx.body.write_sleb128(operand.get_int_immediate());
    } else if (operand.is_symbol()) {
        ctx.relocs.push_back(
            WasmRelocation{
                .type = WasmRelocType::GLOBAL_INDEX_LEB,
                .offset = static_cast<std::uint32_t>(ctx.body.get_size()),
                .index = symbol_indices.at(operand.get_symbol().name),
                .addend = 0,
            }
        );

        write_reloc_placeholder_32(ctx.body);
    }
}

void WasmBuilder::encode_global_set(FuncContext &ctx, mcode::Instruction &instr) {
    mcode::Operand &operand = instr.get_operand(0);

    ctx.body.write_u8(0x24);

    if (operand.is_int_immediate()) {
        ctx.body.write_sleb128(operand.get_int_immediate());
    } else if (operand.is_symbol()) {
        ctx.relocs.push_back(
            WasmRelocation{
                .type = WasmRelocType::GLOBAL_INDEX_LEB,
                .offset = static_cast<std::uint32_t>(ctx.body.get_size()),
                .index = symbol_indices.at(operand.get_symbol().name),
                .addend = 0,
            }
        );

        write_reloc_placeholder_32(ctx.body);
    }
}

void WasmBuilder::encode_i32_load(FuncContext &ctx, mcode::Instruction &instr) {
    ctx.body.write_u8(0x28);
    ctx.body.write_uleb128(0x02);
    encode_load_store_addr(ctx, instr.get_operand(0));
}

void WasmBuilder::encode_i64_load(FuncContext &ctx, mcode::Instruction &instr) {
    ctx.body.write_u8(0x29);
    ctx.body.write_uleb128(0x03);
    encode_load_store_addr(ctx, instr.get_operand(0));
}

void WasmBuilder::encode_f32_load(FuncContext &ctx, mcode::Instruction &instr) {
    ctx.body.write_u8(0x2A);
    ctx.body.write_uleb128(0x02);
    encode_load_store_addr(ctx, instr.get_operand(0));
}

void WasmBuilder::encode_f64_load(FuncContext &ctx, mcode::Instruction &instr) {
    ctx.body.write_u8(0x2B);
    ctx.body.write_uleb128(0x03);
    encode_load_store_addr(ctx, instr.get_operand(0));
}

void WasmBuilder::encode_i32_store(FuncContext &ctx, mcode::Instruction &instr) {
    ctx.body.write_u8(0x36);
    ctx.body.write_uleb128(0x02);
    encode_load_store_addr(ctx, instr.get_operand(0));
}

void WasmBuilder::encode_i64_store(FuncContext &ctx, mcode::Instruction &instr) {
    ctx.body.write_u8(0x37);
    ctx.body.write_uleb128(0x03);
    encode_load_store_addr(ctx, instr.get_operand(0));
}

void WasmBuilder::encode_f32_store(FuncContext &ctx, mcode::Instruction &instr) {
    ctx.body.write_u8(0x38);
    ctx.body.write_uleb128(0x02);
    encode_load_store_addr(ctx, instr.get_operand(0));
}

void WasmBuilder::encode_f64_store(FuncContext &ctx, mcode::Instruction &instr) {
    ctx.body.write_u8(0x39);
    ctx.body.write_uleb128(0x03);
    encode_load_store_addr(ctx, instr.get_operand(0));
}

void WasmBuilder::encode_i32_store8(FuncContext &ctx, mcode::Instruction &instr) {
    ctx.body.write_u8(0x3A);
    ctx.body.write_uleb128(0x00);
    encode_load_store_addr(ctx, instr.get_operand(0));
}

void WasmBuilder::encode_i32_store16(FuncContext &ctx, mcode::Instruction &instr) {
    ctx.body.write_u8(0x3B);
    ctx.body.write_uleb128(0x01);
    encode_load_store_addr(ctx, instr.get_operand(0));
}

void WasmBuilder::encode_i32_const(FuncContext &ctx, mcode::Instruction &instr) {
    mcode::Operand &operand = instr.get_operand(0);

    ctx.body.write_u8(0x41);

    if (operand.is_int_immediate()) {
        ctx.body.write_sleb128(operand.get_int_immediate());
    } else if (operand.is_symbol()) {
        ctx.relocs.push_back(
            WasmRelocation{
                .type = WasmRelocType::MEMORY_ADDR_SLEB,
                .offset = static_cast<std::uint32_t>(ctx.body.get_size()),
                .index = symbol_indices.at(operand.get_symbol().name),
                .addend = 0,
            }
        );

        write_reloc_placeholder_32(ctx.body);
    } else if (operand.is_stack_slot_offset()) {
        const mcode::Operand::StackSlotOffset &offset = operand.get_stack_slot_offset();
        mcode::StackSlot &slot = ctx.func.get_stack_frame().get_stack_slot(offset.slot_index);
        ctx.body.write_sleb128(slot.get_offset() + offset.addend);
    } else {
        ASSERT_UNREACHABLE;
    }
}

void WasmBuilder::encode_load_store_addr(FuncContext &ctx, mcode::Operand &addr) {
    if (addr.is_int_immediate()) {
        ctx.body.write_uleb128(addr.get_int_immediate().to_u64());
    } else if (addr.is_stack_slot_offset()) {
        const mcode::Operand::StackSlotOffset &offset = addr.get_stack_slot_offset();
        mcode::StackSlot &slot = ctx.func.get_stack_frame().get_stack_slot(offset.slot_index);
        ctx.body.write_uleb128(slot.get_offset() + offset.addend);
    } else {
        ASSERT_UNREACHABLE;
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
