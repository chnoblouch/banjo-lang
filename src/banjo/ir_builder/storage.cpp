#include "storage.hpp"

#include <utility>

namespace ir_builder {

const StorageReqs StorageReqs::NONE;
const StorageReqs StorageReqs::FORCE_VALUE(false);
const StorageReqs StorageReqs::FORCE_REFERENCE(true);
const StorageReqs StorageReqs::LOCATION(true);

StoredValue StoredValue::create_value(ir::Value value, IRBuilderContext &context) {
    return {false, true, std::move(value)};
}

StoredValue StoredValue::create_ptr(ir::Value value, IRBuilderContext &context) {
    target::TargetDataLayout &data_layout = context.get_target()->get_data_layout();
    bool fits_in_reg = data_layout.fits_in_register(value.get_type().deref());
    return {true, fits_in_reg, std::move(value)};
}

StoredValue StoredValue::alloc(const ir::Type &type, StorageReqs reqs, IRBuilderContext &context) {
    ir::Value val = reqs.dst ? *reqs.dst : ir::Operand::from_register(context.append_alloca(type));
    val.set_type(type.ref());

    target::TargetDataLayout &data_layout = context.get_target()->get_data_layout();
    bool fits_in_reg = data_layout.fits_in_register(type);

    return {true, fits_in_reg, val};
}
} // namespace ir_builder
