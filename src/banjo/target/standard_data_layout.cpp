#include "standard_data_layout.hpp"

#include "utils/utils.hpp"

namespace target {

StandardDataLayout::StandardDataLayout(int usize, ir::Type usize_type) : TargetDataLayout(usize, usize_type) {}

int StandardDataLayout::get_size(const ir::Type &type, ir::Module &module_) const {
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

            int offset = 0;
            for (const ir::StructureMember &member : struct_.get_members()) {
                int size = get_size(member.type, module_);
                int alignment = get_alignment(member.type, module_);
                offset = Utils::align(offset, alignment) + size;
            }

            int struct_alignment = get_alignment(type, module_);
            offset = Utils::align(offset, struct_alignment);

            return offset * type.get_array_length();
        } else if (type.is_tuple()) {
            int offset = 0;
            for (const ir::Type &tuple_type : type.get_tuple_types()) {
                int size = get_size(tuple_type, module_);
                int alignment = get_alignment(tuple_type, module_);
                offset = Utils::align(offset, alignment) + size;
            }

            int tuple_alignment = get_alignment(type, module_);
            offset = Utils::align(offset, tuple_alignment);

            return offset * type.get_array_length();
        } else {
            return 0;
        }
    } else {
        return usize * type.get_array_length();
    }
}

int StandardDataLayout::get_alignment(const ir::Type &type, ir::Module &module_) const {
    if (type.get_array_length() != 1) {
        return get_alignment(type.base(), module_);
    } else if (type.get_ptr_depth() != 0 || type.is_primitive()) {
        return get_size(type, module_);
    } else if (type.is_struct()) {
        ir::Structure &struct_ = *type.get_struct();

        int alignment = 0;
        for (const ir::StructureMember &member : struct_.get_members()) {
            alignment = std::max(alignment, get_alignment(member.type, module_));
        }
        return alignment;
    } else if (type.is_tuple()) {
        int alignment = 0;
        for (const ir::Type &type : type.get_tuple_types()) {
            alignment = std::max(alignment, get_alignment(type, module_));
        }
        return alignment;
    }

    return 0;
}

int StandardDataLayout::get_member_offset(ir::Structure *struct_, int index, ir::Module &module_) const {
    int offset = 0;

    for (int i = 0; i < index; i++) {
        const ir::Type &type = struct_->get_members()[i].type;
        int size = get_size(type, module_);
        int alignment = get_alignment(type, module_);
        offset = Utils::align(offset, alignment) + size;
    }

    const ir::Type &type = struct_->get_members()[index].type;
    int alignment = get_alignment(type, module_);
    return Utils::align(offset, alignment);
}

int StandardDataLayout::get_member_offset(const std::vector<ir::Type> &types, int index, ir::Module &module_) const {
    int offset = 0;

    for (int i = 0; i < index; i++) {
        const ir::Type &type = types[i];
        int size = get_size(type, module_);
        int alignment = get_alignment(type, module_);
        offset = Utils::align(offset, alignment) + size;
    }

    const ir::Type &type = types[index];
    int alignment = get_alignment(type, module_);
    return Utils::align(offset, alignment);
}

} // namespace target
