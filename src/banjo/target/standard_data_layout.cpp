#include "standard_data_layout.hpp"

#include "utils/utils.hpp"
#include <utility>

namespace target {

StandardDataLayout::StandardDataLayout(unsigned usize, ir::Type usize_type)
  : TargetDataLayout(usize, std::move(usize_type)) {}

unsigned StandardDataLayout::get_size(const ir::Type &type) const {
    if (type.get_ptr_depth() == 0) {
        if (type.is_primitive()) {
            switch (type.get_primitive()) {
                case ir::Primitive::VOID: return 0;
                case ir::Primitive::I8: return 1 * type.get_array_length();
                case ir::Primitive::I16: return 2 * type.get_array_length();
                case ir::Primitive::I32: return 4 * type.get_array_length();
                case ir::Primitive::I64: return 8 * type.get_array_length();
                case ir::Primitive::F32: return 4 * type.get_array_length();
                case ir::Primitive::F64: return 8 * type.get_array_length();
            }
        } else if (type.is_struct()) {
            ir::Structure &struct_ = *type.get_struct();

            unsigned offset = 0;
            for (const ir::StructureMember &member : struct_.get_members()) {
                unsigned size = get_size(member.type);
                unsigned alignment = get_alignment(member.type);
                offset = Utils::align(offset, alignment) + size;
            }

            unsigned struct_alignment = get_alignment(type);
            offset = Utils::align(offset, struct_alignment);

            return offset * type.get_array_length();
        } else if (type.is_tuple()) {
            unsigned offset = 0;
            for (const ir::Type &tuple_type : type.get_tuple_types()) {
                unsigned size = get_size(tuple_type);
                unsigned alignment = get_alignment(tuple_type);
                offset = Utils::align(offset, alignment) + size;
            }

            unsigned tuple_alignment = get_alignment(type);
            offset = Utils::align(offset, tuple_alignment);

            return offset * type.get_array_length();
        } else {
            return 0;
        }
    } else {
        return usize * type.get_array_length();
    }
}

unsigned StandardDataLayout::get_alignment(const ir::Type &type) const {
    if (type.get_array_length() != 1) {
        return get_alignment(type.base());
    } else if (type.get_ptr_depth() != 0 || type.is_primitive()) {
        return get_size(type);
    } else if (type.is_struct()) {
        ir::Structure &struct_ = *type.get_struct();

        unsigned alignment = 0;
        for (const ir::StructureMember &member : struct_.get_members()) {
            alignment = std::max(alignment, get_alignment(member.type));
        }
        return alignment;
    } else if (type.is_tuple()) {
        unsigned alignment = 0;
        for (const ir::Type &type : type.get_tuple_types()) {
            alignment = std::max(alignment, get_alignment(type));
        }
        return alignment;
    }

    return 0;
}

unsigned StandardDataLayout::get_member_offset(ir::Structure *struct_, unsigned index) const {
    unsigned offset = 0;

    for (unsigned i = 0; i < index; i++) {
        const ir::Type &type = struct_->get_members()[i].type;
        unsigned size = get_size(type);
        unsigned alignment = get_alignment(type);
        offset = Utils::align(offset, alignment) + size;
    }

    const ir::Type &type = struct_->get_members()[index].type;
    unsigned alignment = get_alignment(type);
    return Utils::align(offset, alignment);
}

unsigned StandardDataLayout::get_member_offset(const std::vector<ir::Type> &types, unsigned index) const {
    unsigned offset = 0;

    for (unsigned i = 0; i < index; i++) {
        const ir::Type &type = types[i];
        unsigned size = get_size(type);
        unsigned alignment = get_alignment(type);
        offset = Utils::align(offset, alignment) + size;
    }

    const ir::Type &type = types[index];
    unsigned alignment = get_alignment(type);
    return Utils::align(offset, alignment);
}

} // namespace target
