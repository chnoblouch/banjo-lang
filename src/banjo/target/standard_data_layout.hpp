#ifndef STANDARD_DATA_LAYOUT_H
#define STANDARD_DATA_LAYOUT_H

#include "banjo/target/target_data_layout.hpp"

namespace banjo {

namespace target {

class StandardDataLayout : public TargetDataLayout {

public:
    StandardDataLayout(unsigned usize, ssa::Type usize_type);
    unsigned get_size(const ssa::Type &type) const override final;
    unsigned get_alignment(const ssa::Type &type) const override final;
    unsigned get_member_offset(ssa::Structure *struct_, unsigned index) const override final;
    unsigned get_member_offset(const std::vector<ssa::Type> &types, unsigned index) const override final;
};

} // namespace target

} // namespace banjo

#endif
