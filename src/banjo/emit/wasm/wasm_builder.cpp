#include "wasm_builder.hpp"

#include "banjo/emit/wasm/wasm_format.hpp"
#include "banjo/mcode/function.hpp"
#include "banjo/ssa/primitive.hpp"
#include "banjo/target/wasm/wasm_mcode.hpp"
#include "banjo/target/wasm/wasm_opcode.hpp"
#include "banjo/utils/macros.hpp"

#include <any>

namespace banjo {

WasmObjectFile WasmBuilder::build(mcode::Module &mod) {
    WasmObjectFile file{
        .imports{
            WasmImport{
                .mod = "env",
                .name = "__linear_memory",
                .type{
                    WasmMemory{
                        .min_size = 0,
                    },
                }
            },
        },
    };

    for (mcode::Function *func : mod.get_functions()) {
        if (func->get_name() != "add" && func->get_name() != "fadd") {
            continue;
        }

        build_func(file, *func);
    }

    return file;
}

void WasmBuilder::build_func(WasmObjectFile &file, mcode::Function &func) {
    file.types.push_back(build_func_type(func));

    file.functions.push_back(
        WasmFunction{
            .type_index = static_cast<std::uint32_t>(file.types.size() - 1),
            .local_groups = build_local_groups(func),
            .body = encode_instrs(func),
        }
    );

    file.symbols.push_back(
        WasmSymbol{
            .type = WasmSymbolType::FUNCTION,
            .flags = WasmSymbolFlags::EXPORTED,
            .index = static_cast<std::uint32_t>(file.functions.size() - 1),
            .name = func.get_name(),
        }
    );
}

std::vector<std::uint8_t> WasmBuilder::encode_instrs(mcode::Function &func) {
    WriteBuffer buffer;

    for (mcode::BasicBlock &block : func) {
        for (mcode::Instruction &instr : block) {
            encode_instr(buffer, instr);
        }
    }

    return buffer.move_data();
}

void WasmBuilder::encode_instr(WriteBuffer &buffer, mcode::Instruction &instr) {
    switch (instr.get_opcode()) {
        case target::WasmOpcode::END: buffer.write_u8(0x0B); break;
        case target::WasmOpcode::LOCAL_GET:
            buffer.write_u8(0x20);
            buffer.write_uleb128(instr.get_operand(0).get_int_immediate().to_u64());
            break;
        case target::WasmOpcode::LOCAL_SET:
            buffer.write_u8(0x21);
            buffer.write_uleb128(instr.get_operand(0).get_int_immediate().to_u64());
            break;
        case target::WasmOpcode::I32_CONST:
            buffer.write_u8(0x41);
            buffer.write_sleb128(instr.get_operand(0).get_int_immediate());
            break;
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

WasmFunctionType WasmBuilder::build_func_type(mcode::Function &func) {
    const target::WasmFunctionData &func_data = std::any_cast<target::WasmFunctionData>(func.get_target_data());

    WasmFunctionType type;
    type.param_types.resize(func_data.params.size());

    for (unsigned i = 0; i < func_data.params.size(); i++) {
        type.param_types[i] = build_value_type(func_data.params[i]);
    }

    if (func_data.result_type) {
        type.result_types.push_back(build_value_type(*func_data.result_type));
    }

    return type;
}

std::vector<WasmLocalGroup> WasmBuilder::build_local_groups(mcode::Function &func) {
    const target::WasmFunctionData &func_data = std::any_cast<target::WasmFunctionData>(func.get_target_data());

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

} // namespace banjo
