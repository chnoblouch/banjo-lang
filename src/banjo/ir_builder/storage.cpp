#include "storage.hpp"
#include "ir_builder_context.hpp"

#include <cassert>
#include <utility>

namespace ir_builder {

const StorageHints StorageHints::NONE;
const StorageHints StorageHints::PREFER_REFERENCE(true);

StoredValue StoredValue::create_value(ir::Value value) {
    return {false, value.get_type(), std::move(value)};
}

StoredValue StoredValue::create_value(ir::VirtualRegister reg, ir::Type value_type) {
    ir::Value value = ir::Value::from_register(reg, value_type);
    return {false, std::move(value_type), value};
}

StoredValue StoredValue::create_reference(ir::Value value, ir::Type value_type) {
    return {true, std::move(value_type), std::move(value)};
}

StoredValue StoredValue::create_reference(ir::VirtualRegister reg, ir::Type value_type) {
    ir::Value value = ir::Value::from_register(reg, value_type.ref());
    return {true, std::move(value_type), std::move(value)};
}

StoredValue StoredValue::alloc(const ir::Type &type, const StorageHints &hints, IRBuilderContext &context) {
    ir::Value val = hints.dst ? *hints.dst : ir::Operand::from_register(context.append_alloca(type));
    return StoredValue::create_reference(val.with_type(type.ref()), type);
}

bool StoredValue::fits_in_reg(IRBuilderContext &context) {
    return context.get_target()->get_data_layout().fits_in_register(value_type);
}

StoredValue StoredValue::turn_into_reference(IRBuilderContext &context) {
    if (reference) {
        return *this;
    } else {
        ir::VirtualRegister dst_reg = context.append_alloca(value_type);
        ir::Value dst = ir::Value::from_register(dst_reg, value_type.ref());
        context.append_store(value_or_ptr, dst);
        return create_reference(dst, value_type);
    }
}

StoredValue StoredValue::try_turn_into_value(IRBuilderContext &context) {
    if (reference && fits_in_reg(context)) {
        ir::Value val = context.append_load(value_type, value_or_ptr);
        return StoredValue::create_value(val);
    } else {
        return *this;
    }
}

StoredValue StoredValue::turn_into_value(IRBuilderContext &context) {
    if (reference) {
        assert(fits_in_reg(context));
        ir::Value val = context.append_load(value_type, value_or_ptr);
        return StoredValue::create_value(val);
    } else {
        return *this;
    }
}

void StoredValue::copy_to(const ir::Value &dst, IRBuilderContext &context) {
    if (value_or_ptr.is_register() && value_or_ptr == dst) {
        return;
    }

    if (reference) {
        if (fits_in_reg(context)) {
            ir::Value val = context.append_load(value_type, value_or_ptr);
            context.append_store(val, dst);
        } else {
            const ir::Value &copy_src = value_or_ptr;
            ir::Value copy_dst = dst.with_type(copy_src.get_type());
            context.append_copy(copy_dst, copy_src, value_type);
        }
    } else {
        context.append_store(value_or_ptr, dst);
    }
}

} // namespace ir_builder
