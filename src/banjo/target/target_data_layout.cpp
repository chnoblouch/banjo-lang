#include "target_data_layout.hpp"

#include <utility>

namespace banjo {

namespace target {

TargetDataLayout::TargetDataLayout(unsigned usize, ssa::Type usize_type)
  : usize(usize),
    usize_type(std::move(usize_type)) {}

bool TargetDataLayout::is_pass_by_ref(const ssa::Type &type) const {
    return get_size(type) > usize;
}

bool TargetDataLayout::is_return_by_ref(const ssa::Type &type) const {
    return get_size(type) > usize;
}

bool TargetDataLayout::fits_in_register(const ssa::Type &type) const {
    return get_size(type) <= usize;
}

} // namespace target

} // namespace banjo
