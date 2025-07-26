#ifndef BANJO_EMIT_WASM_BUILDER_H
#define BANJO_EMIT_WASM_BUILDER_H

#include "banjo/emit/wasm/wasm_format.hpp"

namespace banjo {

class WasmBuilder {

public:
    WasmObjectFile build();
};

} // namespace banjo

#endif
