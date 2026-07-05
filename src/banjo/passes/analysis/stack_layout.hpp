#ifndef BANJO_PASSES_ANALYSIS_STACK_LAYOUT_H
#define BANJO_PASSES_ANALYSIS_STACK_LAYOUT_H

#include "banjo/ssa/function.hpp"
#include "banjo/ssa/type.hpp"
#include "banjo/ssa/virtual_register.hpp"
#include "banjo/target/target.hpp"

#include <optional>
#include <vector>

namespace banjo::passes {

class StackLayout {

public:
    struct Member {
        unsigned index;
        ssa::VirtualRegister slot;
        unsigned offset;
        ssa::Type type;
    };

    struct Slot {
        std::vector<Member> members;
    };

private:
    target::Target &target;
    ssa::Function *func;
    std::unordered_map<ssa::VirtualRegister, Slot> slots;
    unsigned num_members;

public:
    StackLayout(target::Target &target);

    void build(ssa::Function &func);
    Member *find_member(ssa::VirtualRegister reg);
    std::optional<ssa::VirtualRegister> find_base_slot(ssa::VirtualRegister reg);
    bool is_non_overlapping(ssa::VirtualRegister a, ssa::VirtualRegister b, unsigned size);

    unsigned get_num_members() const { return num_members; }

private:
    void collect_members(ssa::VirtualRegister slot_reg, Slot &slot, ssa::Type type, unsigned base_offset);
};

} // namespace banjo::passes

#endif
