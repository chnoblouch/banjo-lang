#ifndef BANJO_TARGET_TARGET_DATA_LAYOUT_H
#define BANJO_TARGET_TARGET_DATA_LAYOUT_H

#include "banjo/ssa/structure.hpp"
#include "banjo/ssa/type.hpp"

namespace banjo {

namespace target {

class TargetDataLayout {

protected:
    unsigned register_size;
    ssa::Type usize_type;
    bool supports_structs_in_regs;

public:
    TargetDataLayout(unsigned register_size, ssa::Type usize_type, bool supports_structs_in_regs);

    virtual ~TargetDataLayout() = default;
    virtual unsigned get_size(const ssa::Type &type) const = 0;
    virtual unsigned get_alignment(const ssa::Type &type) const = 0;
    virtual unsigned get_member_offset(ssa::Structure *struct_, unsigned index) const = 0;

    ssa::Type get_usize_type() const { return usize_type; }
    bool fits_in_register(const ssa::Type &type) const;

private:
    static bool is_single_member(const ssa::Structure &struct_);
};

} // namespace target

} // namespace banjo

#endif
