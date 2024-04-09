#ifndef STANDARD_DATA_LAYOUT_H
#define STANDARD_DATA_LAYOUT_H

#include "target/target_data_layout.hpp"

namespace target {

class StandardDataLayout : public TargetDataLayout {

public:
    StandardDataLayout(int usize, ir::Type usize_type);
    int get_size(const ir::Type &type, ir::Module &module_) const;
    int get_alignment(const ir::Type &type, ir::Module &module_) const;
    int get_member_offset(ir::Structure *struct_, int index, ir::Module &module_) const;
    int get_member_offset(const std::vector<ir::Type> &types, int index, ir::Module &module_) const;
};

} // namespace target

#endif
