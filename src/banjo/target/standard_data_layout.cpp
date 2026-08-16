#include "standard_data_layout.hpp"

#include "banjo/utils/macros.hpp"
#include "banjo/utils/utils.hpp"

#include <algorithm>

namespace banjo::target {

StandardDataLayout::StandardDataLayout(Params params) : TargetDataLayout(params) {}

unsigned StandardDataLayout::get_size(const ssa::Type &type) const {
    if (type.is_primitive()) {
        switch (type.get_primitive()) {
            case ssa::Primitive::VOID: return 0;
            case ssa::Primitive::U8: return 1 * type.get_array_length();
            case ssa::Primitive::U16: return 2 * type.get_array_length();
            case ssa::Primitive::U32: return 4 * type.get_array_length();
            case ssa::Primitive::U64: return 8 * type.get_array_length();
            case ssa::Primitive::I8: return 1 * type.get_array_length();
            case ssa::Primitive::I16: return 2 * type.get_array_length();
            case ssa::Primitive::I32: return 4 * type.get_array_length();
            case ssa::Primitive::I64: return 8 * type.get_array_length();
            case ssa::Primitive::F32: return 4 * type.get_array_length();
            case ssa::Primitive::F64: return 8 * type.get_array_length();
            case ssa::Primitive::ADDR: return get_size(get_usize_type()) * type.get_array_length();
        }
    } else if (type.is_struct()) {
        ssa::Structure &struct_ = *type.get_struct();
        unsigned size = 0;

        if (!struct_.is_union) {
            for (const ssa::StructureMember &member : struct_.members) {
                unsigned member_size = get_size(member.type);
                unsigned member_alignment = get_alignment(member.type);
                size = utils::align(size, member_alignment) + member_size;
            }
        } else {
            for (const ssa::StructureMember &member : struct_.members) {
                size = std::max(size, get_size(member.type));
            }
        }

        unsigned struct_alignment = get_alignment(type);
        size = utils::align(size, struct_alignment);

        return size * type.get_array_length();
    } else {
        ASSERT_UNREACHABLE;
    }
}

unsigned StandardDataLayout::get_alignment(const ssa::Type &type) const {
    if (type.is_primitive()) {
        return get_size(type.get_primitive());
    } else if (type.is_struct()) {
        ssa::Structure &struct_ = *type.get_struct();

        if (struct_.members.empty()) {
            return 1;
        }

        unsigned alignment = 0;
        for (const ssa::StructureMember &member : struct_.members) {
            alignment = std::max(alignment, get_alignment(member.type));
        }
        return alignment;
    } else {
        ASSERT_UNREACHABLE;
    }
}

unsigned StandardDataLayout::get_member_offset(ssa::Structure *struct_, unsigned index) const {
    ASSERT(!struct_->is_union);

    unsigned offset = 0;

    for (unsigned i = 0; i < index; i++) {
        const ssa::Type &type = struct_->members[i].type;
        unsigned size = get_size(type);
        unsigned alignment = get_alignment(type);
        offset = utils::align(offset, alignment) + size;
    }

    const ssa::Type &type = struct_->members[index].type;
    unsigned alignment = get_alignment(type);
    return utils::align(offset, alignment);
}

} // namespace banjo::target
