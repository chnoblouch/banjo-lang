#include "target_data_layout.hpp"

#include "banjo/ssa/structure.hpp"
#include "banjo/utils/macros.hpp"
#include "banjo/utils/utils.hpp"
#include <algorithm>

namespace banjo::target {

TargetDataLayout::TargetDataLayout(Params params) : params(params) {}

bool TargetDataLayout::fits_in_register(const ssa::Type &type) const {
    if (type.is_struct() && !params.supports_structs_in_regs) {
        return is_single_member(*type.get_struct());
    }

    return get_size(type) <= params.register_size;
}

ArgPassMethod TargetDataLayout::get_arg_pass_method(const ssa::Type &type) const {
    if (type.is_struct() && !params.supports_structs_in_regs) {
        if (is_single_member(*type.get_struct())) {
            return ArgPassMethod{.via_pointer = false, .num_args = 1, .last_arg_type = type};
        } else {
            return ArgPassMethod{.via_pointer = true, .num_args = 1, .last_arg_type = get_usize_type()};
        }
    }

    unsigned size = get_size(type);
    unsigned num_args = std::max(utils::div_ceil(size, params.register_size), 1u);

    if (num_args == 1) {
        return ArgPassMethod{
            .via_pointer = false,
            .num_args = 1,
            .last_arg_type = type,
        };
    } else if (num_args <= params.max_regs_per_arg) {
        unsigned last_arg_size = size % params.register_size;

        if (last_arg_size == 0) {
            last_arg_size = params.register_size;
        }

        return ArgPassMethod{
            .via_pointer = false,
            .num_args = num_args,
            .last_arg_type = ssa::Type::sized(last_arg_size),
        };
    } else {
        return ArgPassMethod{.via_pointer = true, .num_args = 1, .last_arg_type = get_usize_type()};
    }
}

bool TargetDataLayout::is_single_member(const ssa::Structure &struct_) {
    if (struct_.members.size() == 1) {
        const ssa::Type &member_type = struct_.members[0].type;

        if (member_type.get_array_length() > 1) {
            return false;
        }

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
