#ifndef VALUE_STORAGE_H
#define VALUE_STORAGE_H

#include "ir/operand.hpp"
#include "ir_builder/ir_builder_context.hpp"

#include <optional>

namespace ir_builder {

struct StorageReqs {
    static const StorageReqs NONE;
    static const StorageReqs FORCE_VALUE;
    static const StorageReqs FORCE_REFERENCE;
    static const StorageReqs LOCATION;

    std::optional<ir::Value> dst;
    bool force_reference;

    StorageReqs() : dst(), force_reference(false) {}
    StorageReqs(ir::Value dst) : dst(dst), force_reference(false) {}
    StorageReqs(ir::VirtualRegister reg) : dst(ir::Value::from_register(reg)), force_reference(false) {}
    StorageReqs(bool force_reference) : force_reference(force_reference) {}
};

struct StoredValue {
    bool reference;
    bool fits_in_reg;
    ir::Value value_or_ptr;

    static StoredValue create_value(ir::Value value, IRBuilderContext &context);
    static StoredValue create_ptr(ir::Value value, IRBuilderContext &context);
    static StoredValue alloc(const ir::Type &type, StorageReqs reqs, IRBuilderContext &context);
};

} // namespace ir_builder

#endif
