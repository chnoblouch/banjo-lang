#include "ir_builder.hpp"

namespace banjo {

namespace ir_builder {

unsigned IRBuilder::get_size(const ir::Type &type) {
    return context.get_target()->get_data_layout().get_size(type);
}

bool IRBuilder::is_pass_by_ref(const ir::Type &type) {
    return context.get_target()->get_data_layout().is_pass_by_ref(type);
}

bool IRBuilder::is_return_by_ref(const ir::Type &type) {
    return context.get_target()->get_data_layout().is_return_by_ref(type);
}

} // namespace ir_builder

} // namespace banjo
