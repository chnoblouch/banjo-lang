#ifndef SSA_GEN_STORED_VALUE_H
#define SSA_GEN_STORED_VALUE_H

#include "banjo/ir/operand.hpp"
#include "banjo/ir/virtual_register.hpp"
#include "banjo/ssa_gen/ssa_generator_context.hpp"
#include "banjo/ssa_gen/storage_hints.hpp"

#include <cassert>

namespace banjo {

namespace lang {

struct StoredValue {
    bool reference;
    ir::Type value_type;
    ir::Value value_or_ptr;

    static StoredValue create_value(ir::Value value);
    static StoredValue create_value(ir::VirtualRegister reg, ir::Type value_type);

    static StoredValue create_reference(ir::Value value, ir::Type value_type);
    static StoredValue create_reference(ir::VirtualRegister reg, ir::Type value_type);

    static StoredValue alloc(const ir::Type &type, const StorageHints &hints, SSAGeneratorContext &ctx);

    const ir::Value &get_value() const {
        assert(!reference);
        return value_or_ptr;
    }

    const ir::Value &get_ptr() const {
        assert(reference);
        return value_or_ptr;
    }

    bool fits_in_reg(SSAGeneratorContext &ctx);
    StoredValue turn_into_reference(SSAGeneratorContext &ctx);
    StoredValue try_turn_into_value(SSAGeneratorContext &ctx);
    StoredValue turn_into_value(SSAGeneratorContext &ctx);
    void copy_to(const ir::Value &dst, SSAGeneratorContext &ctx);
};

} // namespace lang

} // namespace banjo

#endif
