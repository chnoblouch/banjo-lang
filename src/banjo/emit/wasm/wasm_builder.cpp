#include "wasm_builder.hpp"

#include "banjo/emit/wasm/wasm_format.hpp"
#include "banjo/mcode/function.hpp"
#include "banjo/ssa/primitive.hpp"
#include "banjo/target/wasm/wasm_opcode.hpp"
#include "banjo/utils/macros.hpp"

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
        case target::WasmOpcode::I32_ADD: buffer.write_u8(0x6A); break;
        case target::WasmOpcode::F32_ADD: buffer.write_u8(0x92); break;
        default: ASSERT_UNREACHABLE;
    }
}

WasmFunctionType WasmBuilder::build_func_type(mcode::Function &func) {
    WasmFunctionType type;
    type.param_types.resize(func.get_parameters().size());

    for (unsigned i = 0; i < func.get_parameters().size(); i++) {
        type.param_types[i] = build_value_type(func.get_parameters()[i].type);
    }

    if (func.get_parameters()[0].type.is_floating_point()) {
        type.result_types.push_back(build_value_type(ssa::Primitive::F32));
    } else {
        type.result_types.push_back(build_value_type(ssa::Primitive::I32));
    }

    return type;
}

std::uint8_t WasmBuilder::build_value_type(ssa::Type type) {
    ASSERT(type.is_primitive() && type.get_array_length() == 1);

    switch (type.get_primitive()) {
        case ssa::Primitive::VOID: ASSERT_UNREACHABLE;
        case ssa::Primitive::I8:
        case ssa::Primitive::I16:
        case ssa::Primitive::I32: return WasmValueType::I32;
        case ssa::Primitive::I64: return WasmValueType::I64;
        case ssa::Primitive::F32: return WasmValueType::F32;
        case ssa::Primitive::F64: return WasmValueType::F64;
        case ssa::Primitive::ADDR: return WasmValueType::I32;
    }
}

} // namespace banjo
