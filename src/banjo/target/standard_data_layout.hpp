#ifndef STANDARD_DATA_LAYOUT_H
#define STANDARD_DATA_LAYOUT_H

#include "target/target_data_layout.hpp"

namespace banjo {

namespace target {

class StandardDataLayout : public TargetDataLayout {

public:
    StandardDataLayout(unsigned usize, ir::Type usize_type);
    unsigned get_size(const ir::Type &type) const override final;
    unsigned get_alignment(const ir::Type &type) const override final;
    unsigned get_member_offset(ir::Structure *struct_, unsigned index) const override final;
    unsigned get_member_offset(const std::vector<ir::Type> &types, unsigned index) const override final;
};

} // namespace target

} // namespace banjo

#endif
