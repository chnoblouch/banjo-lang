#ifndef TARGET_DATA_LAYOUT_H
#define TARGET_DATA_LAYOUT_H

#include "banjo/ssa/structure.hpp"
#include "banjo/ssa/type.hpp"

#include <vector>

namespace banjo {

namespace target {

class TargetDataLayout {

protected:
    unsigned usize;
    ssa::Type usize_type;

public:
    TargetDataLayout(unsigned usize, ssa::Type usize_type);

    virtual ~TargetDataLayout() = default;
    virtual unsigned get_size(const ssa::Type &type) const = 0;
    virtual unsigned get_alignment(const ssa::Type &type) const = 0;
    virtual unsigned get_member_offset(ssa::Structure *struct_, unsigned index) const = 0;
    virtual unsigned get_member_offset(const std::vector<ssa::Type> &types, unsigned index) const = 0;

    ssa::Type get_usize_type() const { return usize_type; }
    bool is_pass_by_ref(const ssa::Type &type) const;
    bool is_return_by_ref(const ssa::Type &type) const;
    bool fits_in_register(const ssa::Type &type) const;
};

} // namespace target

} // namespace banjo

#endif
