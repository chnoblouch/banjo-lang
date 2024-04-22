#ifndef VALUE_STORAGE_H
#define VALUE_STORAGE_H

#include "ir/operand.hpp"
#include "ir/virtual_register.hpp"
#include "ir_builder/ir_builder_context.hpp"

#include <cassert>
#include <optional>

namespace ir_builder {

struct StorageHints {
    static const StorageHints NONE;
    static const StorageHints PREFER_REFERENCE;

    std::optional<ir::Value> dst;
    bool prefer_reference;

    StorageHints() : dst(), prefer_reference(false) {}
    StorageHints(ir::Value dst) : dst(dst), prefer_reference(false) {}
    StorageHints(ir::VirtualRegister reg) : dst(ir::Value::from_register(reg)), prefer_reference(false) {}
    StorageHints(bool prefer_reference) : prefer_reference(prefer_reference) {}
};

struct StoredValue {
    bool reference;
    ir::Type value_type;
    ir::Value value_or_ptr;

    static StoredValue create_value(ir::Value value);
    static StoredValue create_value(ir::VirtualRegister reg, ir::Type value_type);

    static StoredValue create_reference(ir::Value value, ir::Type value_type);
    static StoredValue create_reference(ir::VirtualRegister reg, ir::Type value_type);

    static StoredValue alloc(const ir::Type &type, const StorageHints &hints, IRBuilderContext &context);

    ir::Value &get_value() {
        assert(!reference);
        return value_or_ptr;
    }

    ir::Value &get_ptr() {
        assert(reference);
        return value_or_ptr;
    }

    bool fits_in_reg(IRBuilderContext &context);
    StoredValue turn_into_reference(IRBuilderContext &context);
    StoredValue try_turn_into_value(IRBuilderContext &context);
    StoredValue turn_into_value(IRBuilderContext &context);
    void copy_to(const ir::Value &dst, IRBuilderContext &context);
};

} // namespace ir_builder

#endif
