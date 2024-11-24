#include "stored_value.hpp"

#include "banjo/utils/macros.hpp"

#include <cassert>

namespace banjo {

namespace lang {

StoredValue StoredValue::create_value(ssa::Value value) {
    return {Kind::VALUE, value.get_type(), std::move(value)};
}

StoredValue StoredValue::create_value(ssa::VirtualRegister reg, ssa::Type value_type) {
    ssa::Value value = ssa::Value::from_register(reg, value_type);
    return {Kind::VALUE, value_type, value};
}

StoredValue StoredValue::create_reference(ssa::Value value, ssa::Type value_type) {
    return {Kind::REFERENCE, value_type, std::move(value)};
}

StoredValue StoredValue::create_reference(ssa::VirtualRegister reg, ssa::Type value_type) {
    ssa::Value value = ssa::Value::from_register(reg, ssa::Primitive::ADDR);
    return {Kind::REFERENCE, value_type, std::move(value)};
}

StoredValue StoredValue::create_undefined(ssa::Type value_type) {
    return {Kind::UNDEFINED, value_type};
}

StoredValue StoredValue::alloc(const ssa::Type &type, const StorageHints &hints, SSAGeneratorContext &ctx) {
    ssa::Value val = hints.dst ? *hints.dst : ssa::Operand::from_register(ctx.append_alloca(type));
    return StoredValue::create_reference(val.with_type(ssa::Primitive::ADDR), type);
}

bool StoredValue::fits_in_reg(SSAGeneratorContext &ctx) {
    return ctx.target->get_data_layout().fits_in_register(value_type);
}

StoredValue StoredValue::turn_into_reference(SSAGeneratorContext &ctx) {
    if (kind == Kind::VALUE) {
        ssa::VirtualRegister dst_reg = ctx.append_alloca(value_type);
        ssa::Value dst = ssa::Value::from_register(dst_reg, ssa::Primitive::ADDR);
        ctx.append_store(value_or_ptr, dst);
        return create_reference(dst, value_type);
    } else if (kind == Kind::REFERENCE) {
        return *this;
    } else {
        ASSERT_UNREACHABLE;
    }
}

StoredValue StoredValue::try_turn_into_value(SSAGeneratorContext &ctx) {
    if (kind == Kind::REFERENCE && fits_in_reg(ctx)) {
        ssa::Value val = ctx.append_load(value_type, value_or_ptr);
        return StoredValue::create_value(val);
    } else {
        return *this;
    }
}

StoredValue StoredValue::turn_into_value(SSAGeneratorContext &ctx) {
    if (kind == Kind::VALUE) {
        return *this;
    } else if (kind == Kind::REFERENCE) {
        ASSERT(fits_in_reg(ctx));
        ssa::Value val = ctx.append_load(value_type, value_or_ptr);
        return StoredValue::create_value(val);
    } else if (kind == Kind::UNDEFINED) {
        ASSERT(fits_in_reg(ctx));
        ssa::Value val = ssa::Value::from_int_immediate(0, value_type);
        return StoredValue::create_value(val);
    } else {
        ASSERT_UNREACHABLE;
    }
}

StoredValue StoredValue::turn_into_value_or_copy(SSAGeneratorContext &ctx) {
    if (kind == Kind::VALUE) {
        return *this;
    } else if (kind == Kind::REFERENCE) {
        if (fits_in_reg(ctx)) {
            ssa::Value val = ctx.append_load(value_type, value_or_ptr);
            return StoredValue::create_value(val);
        } else {
            const ssa::Value &copy_src = value_or_ptr;
            ssa::Value copy_dst = ssa::Operand::from_register(ctx.append_alloca(value_type), ssa::Primitive::ADDR);
            ctx.append_copy(copy_dst, copy_src, value_type);
            return StoredValue::create_reference(copy_dst, value_type);
        }
    } else if (kind == Kind::UNDEFINED) {
        if (fits_in_reg(ctx)) {
            ssa::Value val = ssa::Value::from_int_immediate(0, value_type);
            return StoredValue::create_value(val);
        } else {
            ssa::Value stack_slot = ssa::Operand::from_register(ctx.append_alloca(value_type));
            return StoredValue::create_reference(stack_slot, value_type);
        }
    } else {
        ASSERT_UNREACHABLE;
    }
}

void StoredValue::copy_to(const ssa::Value &dst, SSAGeneratorContext &ctx) {
    if (value_or_ptr.is_register() && value_or_ptr == dst) {
        return;
    }

    if (kind == Kind::VALUE) {
        ctx.append_store(value_or_ptr, dst);
    } else if (kind == Kind::REFERENCE) {
        if (fits_in_reg(ctx)) {
            ssa::Value val = ctx.append_load(value_type, value_or_ptr);
            ctx.append_store(val, dst);
        } else {
            const ssa::Value &copy_src = value_or_ptr;
            ssa::Value copy_dst = dst.with_type(copy_src.get_type());
            ctx.append_copy(copy_dst, copy_src, value_type);
        }
    } else if (kind == Kind::UNDEFINED) {
        // Do nothing.
    }
}

void StoredValue::copy_to(const ssa::VirtualRegister &dst, SSAGeneratorContext &ctx) {
    copy_to(ssa::Value::from_register(dst, ssa::Primitive::ADDR), ctx);
}

} // namespace lang

} // namespace banjo
