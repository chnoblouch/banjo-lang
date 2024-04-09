#include "ir_builder.hpp"

namespace ir_builder {

int IRBuilder::get_size(const ir::Type &type) {
    return context.get_target()->get_data_layout().get_size(type, *context.get_current_mod());
}

bool IRBuilder::is_pass_by_ref(const ir::Type &type) {
    return context.get_target()->get_data_layout().is_pass_by_ref(type, *context.get_current_mod());
}

bool IRBuilder::is_return_by_ref(const ir::Type &type) {
    return context.get_target()->get_data_layout().is_return_by_ref(type, *context.get_current_mod());
}

} // namespace ir_builder
