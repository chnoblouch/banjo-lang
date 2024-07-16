#include "stored_value.hpp"

#include <cassert>

namespace banjo {

namespace lang {

StoredValue StoredValue::create_value(ir::Value value) {
    return {false, value.get_type(), std::move(value)};
}

StoredValue StoredValue::create_value(ir::VirtualRegister reg, ir::Type value_type) {
    ir::Value value = ir::Value::from_register(reg, value_type);
    return {false, value_type, value};
}

StoredValue StoredValue::create_reference(ir::Value value, ir::Type value_type) {
    return {true, value_type, std::move(value)};
}

StoredValue StoredValue::create_reference(ir::VirtualRegister reg, ir::Type value_type) {
    ir::Value value = ir::Value::from_register(reg, ir::Primitive::ADDR);
    return {true, value_type, std::move(value)};
}

StoredValue StoredValue::alloc(const ir::Type &type, const StorageHints &hints, SSAGeneratorContext &ctx) {
    ir::Value val = hints.dst ? *hints.dst : ir::Operand::from_register(ctx.append_alloca(type));
    return StoredValue::create_reference(val.with_type(ir::Primitive::ADDR), type);
}

bool StoredValue::fits_in_reg(SSAGeneratorContext &ctx) {
    return ctx.target->get_data_layout().fits_in_register(value_type);
}

StoredValue StoredValue::turn_into_reference(SSAGeneratorContext &ctx) {
    if (reference) {
        return *this;
    } else {
        ir::VirtualRegister dst_reg = ctx.append_alloca(value_type);
        ir::Value dst = ir::Value::from_register(dst_reg, ir::Primitive::ADDR);
        ctx.append_store(value_or_ptr, dst);
        return create_reference(dst, value_type);
    }
}

StoredValue StoredValue::try_turn_into_value(SSAGeneratorContext &ctx) {
    if (reference && fits_in_reg(ctx)) {
        ir::Value val = ctx.append_load(value_type, value_or_ptr);
        return StoredValue::create_value(val);
    } else {
        return *this;
    }
}

StoredValue StoredValue::turn_into_value(SSAGeneratorContext &ctx) {
    if (reference) {
        assert(fits_in_reg(ctx));
        ir::Value val = ctx.append_load(value_type, value_or_ptr);
        return StoredValue::create_value(val);
    } else {
        return *this;
    }
}

void StoredValue::copy_to(const ir::Value &dst, SSAGeneratorContext &ctx) {
    if (value_or_ptr.is_register() && value_or_ptr == dst) {
        return;
    }

    if (reference) {
        if (fits_in_reg(ctx)) {
            ir::Value val = ctx.append_load(value_type, value_or_ptr);
            ctx.append_store(val, dst);
        } else {
            const ir::Value &copy_src = value_or_ptr;
            ir::Value copy_dst = dst.with_type(copy_src.get_type());
            ctx.append_copy(copy_dst, copy_src, value_type);
        }
    } else {
        ctx.append_store(value_or_ptr, dst);
    }
}

} // namespace lang

} // namespace banjo
