#ifndef TARGET_DATA_LAYOUT_H
#define TARGET_DATA_LAYOUT_H

#include "ir/module.hpp"
#include "ir/structure.hpp"
#include "ir/type.hpp"

#include <vector>

namespace target {

class TargetDataLayout {

protected:
    int usize;
    ir::Type usize_type;

public:
    TargetDataLayout(int usize, ir::Type usize_type);

    virtual ~TargetDataLayout() = default;
    virtual int get_size(const ir::Type &type, ir::Module &module_) const = 0;
    virtual int get_alignment(const ir::Type &type, ir::Module &module_) const = 0;
    virtual int get_member_offset(ir::Structure *struct_, int index, ir::Module &module_) const = 0;
    virtual int get_member_offset(const std::vector<ir::Type> &types, int index, ir::Module &module_) const = 0;

    ir::Type get_usize_type() const { return usize_type; }
    bool is_pass_by_ref(const ir::Type &type, ir::Module &module) const;
    bool is_return_by_ref(const ir::Type &type, ir::Module &module) const;
    bool fits_in_register(const ir::Type &type, ir::Module &module) const;
};

} // namespace target

#endif
