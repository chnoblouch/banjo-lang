#include "target_data_layout.hpp"

namespace banjo::target {

TargetDataLayout::TargetDataLayout(unsigned register_size, ssa::Type usize_type)
  : register_size(register_size),
    usize_type(usize_type) {}

bool TargetDataLayout::fits_in_register(const ssa::Type &type) const {
    return get_size(type) <= register_size;
}

} // namespace banjo::target
