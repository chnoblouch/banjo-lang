#ifndef TARGET_DATA_LAYOUT_H
#define TARGET_DATA_LAYOUT_H

#include "ir/structure.hpp"
#include "ir/type.hpp"

#include <vector>

namespace target {

class TargetDataLayout {

protected:
    unsigned usize;
    ir::Type usize_type;

public:
    TargetDataLayout(unsigned usize, ir::Type usize_type);

    virtual ~TargetDataLayout() = default;
    virtual unsigned get_size(const ir::Type &type) const = 0;
    virtual unsigned get_alignment(const ir::Type &type) const = 0;
    virtual unsigned get_member_offset(ir::Structure *struct_, unsigned index) const = 0;
    virtual unsigned get_member_offset(const std::vector<ir::Type> &types, unsigned index) const = 0;

    ir::Type get_usize_type() const { return usize_type; }
    bool is_pass_by_ref(const ir::Type &type) const;
    bool is_return_by_ref(const ir::Type &type) const;
    bool fits_in_register(const ir::Type &type) const;
};

} // namespace target

#endif
