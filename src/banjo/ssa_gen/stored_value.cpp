#include "stored_value.hpp"

#include "banjo/utils/macros.hpp"

#include <cassert>

namespace banjo {

namespace lang {

StoredValue StoredValue::create_value(ir::Value value) {
    return {Kind::VALUE, value.get_type(), std::move(value)};
}

StoredValue StoredValue::create_value(ir::VirtualRegister reg, ir::Type value_type) {
    ir::Value value = ir::Value::from_register(reg, value_type);
    return {Kind::VALUE, value_type, value};
}

StoredValue StoredValue::create_reference(ir::Value value, ir::Type value_type) {
    return {Kind::REFERENCE, value_type, std::move(value)};
}

StoredValue StoredValue::create_reference(ir::VirtualRegister reg, ir::Type value_type) {
    ir::Value value = ir::Value::from_register(reg, ir::Primitive::ADDR);
    return {Kind::REFERENCE, value_type, std::move(value)};
}

StoredValue StoredValue::create_undefined(ir::Type value_type) {
    return {Kind::UNDEFINED, value_type};
}

StoredValue StoredValue::alloc(const ir::Type &type, const StorageHints &hints, SSAGeneratorContext &ctx) {
    ir::Value val = hints.dst ? *hints.dst : ir::Operand::from_register(ctx.append_alloca(type));
    return StoredValue::create_reference(val.with_type(ir::Primitive::ADDR), type);
}

bool StoredValue::fits_in_reg(SSAGeneratorContext &ctx) {
    return ctx.target->get_data_layout().fits_in_register(value_type);
}

StoredValue StoredValue::turn_into_reference(SSAGeneratorContext &ctx) {
    if (kind == Kind::VALUE) {
        ir::VirtualRegister dst_reg = ctx.append_alloca(value_type);
        ir::Value dst = ir::Value::from_register(dst_reg, ir::Primitive::ADDR);
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
        ir::Value val = ctx.append_load(value_type, value_or_ptr);
        return StoredValue::create_value(val);
    } else {
        return *this;
    }
}

StoredValue StoredValue::turn_into_value(SSAGeneratorContext &ctx) {
    if (kind == Kind::VALUE) {
        return *this;
    } else if (kind == Kind::REFERENCE) {
        assert(fits_in_reg(ctx));
        ir::Value val = ctx.append_load(value_type, value_or_ptr);
        return StoredValue::create_value(val);
    } else if (kind == Kind::UNDEFINED) {
        assert(fits_in_reg(ctx));
        ir::Value val = ir::Value::from_int_immediate(0, value_type);
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
            ir::Value val = ctx.append_load(value_type, value_or_ptr);
            return StoredValue::create_value(val);
        } else {
            const ir::Value &copy_src = value_or_ptr;
            ir::Value copy_dst = ir::Operand::from_register(ctx.append_alloca(value_type));
            ctx.append_copy(copy_dst, copy_src, value_type);
            return StoredValue::create_reference(copy_dst, value_type);
        }
    } else if (kind == Kind::UNDEFINED) {
        if (fits_in_reg(ctx)) {
            ir::Value val = ir::Value::from_int_immediate(0, value_type);
            return StoredValue::create_value(val);
        } else {
            ir::Value stack_slot = ir::Operand::from_register(ctx.append_alloca(value_type));
            return StoredValue::create_reference(stack_slot, value_type);
        }
    } else {
        ASSERT_UNREACHABLE;
    }
}

void StoredValue::copy_to(const ir::Value &dst, SSAGeneratorContext &ctx) {
    if (value_or_ptr.is_register() && value_or_ptr == dst) {
        return;
    }

    if (kind == Kind::VALUE) {
        ctx.append_store(value_or_ptr, dst);
    } else if (kind == Kind::REFERENCE) {
        if (fits_in_reg(ctx)) {
            ir::Value val = ctx.append_load(value_type, value_or_ptr);
            ctx.append_store(val, dst);
        } else {
            const ir::Value &copy_src = value_or_ptr;
            ir::Value copy_dst = dst.with_type(copy_src.get_type());
            ctx.append_copy(copy_dst, copy_src, value_type);
        }
    } else if (kind == Kind::UNDEFINED) {
        // Do nothing.
    }
}

} // namespace lang

} // namespace banjo
