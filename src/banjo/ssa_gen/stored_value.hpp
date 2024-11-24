#ifndef SSA_GEN_STORED_VALUE_H
#define SSA_GEN_STORED_VALUE_H

#include "banjo/ssa/operand.hpp"
#include "banjo/ssa/virtual_register.hpp"
#include "banjo/ssa_gen/ssa_generator_context.hpp"
#include "banjo/ssa_gen/storage_hints.hpp"

#include <cassert>

namespace banjo {

namespace lang {

struct StoredValue {
    enum class Kind {
        VALUE,
        REFERENCE,
        UNDEFINED,
    };

    Kind kind;
    ssa::Type value_type;
    ssa::Value value_or_ptr;

    static StoredValue create_value(ssa::Value value);
    static StoredValue create_value(ssa::VirtualRegister reg, ssa::Type value_type);
    static StoredValue create_reference(ssa::Value value, ssa::Type value_type);
    static StoredValue create_reference(ssa::VirtualRegister reg, ssa::Type value_type);
    static StoredValue create_undefined(ssa::Type value_type);
    static StoredValue alloc(const ssa::Type &type, const StorageHints &hints, SSAGeneratorContext &ctx);

    const ssa::Value &get_value() const {
        assert(kind == Kind::VALUE);
        return value_or_ptr;
    }

    const ssa::Value &get_ptr() const {
        assert(kind == Kind::REFERENCE);
        return value_or_ptr;
    }

    bool fits_in_reg(SSAGeneratorContext &ctx);
    StoredValue turn_into_reference(SSAGeneratorContext &ctx);
    StoredValue try_turn_into_value(SSAGeneratorContext &ctx);
    StoredValue turn_into_value(SSAGeneratorContext &ctx);
    StoredValue turn_into_value_or_copy(SSAGeneratorContext &ctx);
    void copy_to(const ssa::Value &dst, SSAGeneratorContext &ctx);
    void copy_to(const ssa::VirtualRegister &dst, SSAGeneratorContext &ctx);
};

} // namespace lang

} // namespace banjo

#endif
