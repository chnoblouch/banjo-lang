#include "wasm_builder.hpp"

#include "banjo/emit/wasm/wasm_format.hpp"

namespace banjo {

WasmObjectFile WasmBuilder::build() {
    return WasmObjectFile{
        .types{
            WasmFunctionType{
                .param_types{WasmValueType::I32, WasmValueType::I32},
                .result_types{WasmValueType::I32},
            },
            WasmFunctionType{
                .param_types{WasmValueType::F32, WasmValueType::F32},
                .result_types{WasmValueType::F32},
            },
        },
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
        .functions{
            WasmFunction{
                .type_index = 0,
                .body{
                    0x20, // local.get 1
                    0x01,
                    0x20, // local.get 0
                    0x00,
                    0x6A, // i32.add
                    0x0B, // end
                },
            },
            WasmFunction{
                .type_index = 1,
                .body{
                    0x20, // local.get 1
                    0x01,
                    0x20, // local.get 0
                    0x00,
                    0x92, // f32.add
                    0x0B, // end
                },
            },
        },
        .symbols{
            WasmSymbol{
                .type = WasmSymbolType::FUNCTION,
                .flags = WasmSymbolFlags::EXPORTED,
                .index = 0,
                .name = "add",
            },
            WasmSymbol{
                .type = WasmSymbolType::FUNCTION,
                .flags = WasmSymbolFlags::EXPORTED,
                .index = 1,
                .name = "fadd",
            },
        },
    };
}

} // namespace banjo
