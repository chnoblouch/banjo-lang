#include "target_data_layout.hpp"

#include "banjo/ssa/structure.hpp"
#include "banjo/utils/macros.hpp"

namespace banjo::target {

TargetDataLayout::TargetDataLayout(unsigned register_size, ssa::Type usize_type, bool supports_structs_in_regs)
  : register_size(register_size),
    usize_type(usize_type),
    supports_structs_in_regs(supports_structs_in_regs) {}

bool TargetDataLayout::fits_in_register(const ssa::Type &type) const {
    if (type.is_struct() && !supports_structs_in_regs) {
        return is_single_member(*type.get_struct());
    }

    return get_size(type) <= register_size;
}

bool TargetDataLayout::is_single_member(const ssa::Structure &struct_) {
    if (struct_.members.size() == 1) {
        const ssa::Type &member_type = struct_.members[0].type;

        if (member_type.is_primitive()) {
            return true;
        } else if (member_type.is_struct()) {
            return is_single_member(*member_type.get_struct());
        } else {
            ASSERT_UNREACHABLE;
        }
    } else {
        return false;
    }
}

} // namespace banjo::target
