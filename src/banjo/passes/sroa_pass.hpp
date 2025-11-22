#ifndef BANJO_PASSES_SROA_PASS_H
#define BANJO_PASSES_SROA_PASS_H

#include "banjo/passes/pass.hpp"
#include "banjo/passes/pass_utils.hpp"
#include "banjo/ssa/virtual_register.hpp"

#include <functional>
#include <optional>

namespace banjo {

namespace passes {

class SROAPass : public Pass {

private:
    struct StackValue {
        ssa::InstrIter alloca_instr;
        ssa::BasicBlockIter alloca_block;
        ssa::Type type;
        std::optional<unsigned> parent;
        std::optional<std::vector<unsigned>> members;
        std::optional<ssa::VirtualRegister> replacement;
    };

    struct InsertionContext {
        ssa::Function *func;
        ssa::BasicBlock &block;
        ssa::InstrIter instr;
    };

    struct Ref {
        std::optional<ssa::VirtualRegister> ptr;
        std::optional<unsigned> stack_value;
    };

    std::vector<StackValue> stack_values;
    std::unordered_map<ssa::VirtualRegister, unsigned> stack_ptr_defs;
    PassUtils::UseMap use_map;

public:
    SROAPass(target::Target *target);
    void run(ssa::Module &mod);

private:
    void run(ssa::Function *func);
    void collect_stack_values(ssa::BasicBlockIter block_iter);
    void collect_members(unsigned val_index);
    void disable_invalid_splits(ssa::BasicBlock &block);
    void split_root(StackValue &value, ssa::Function *func);
    void split_member(StackValue &value, ssa::Function *func);

    void collect_stack_ptr_defs(ssa::Function &func);
    void collect_member_ptr_defs(PassUtils::UseMap &use_map, ssa::VirtualRegister base, StackValue &value);

    void split_copies(ssa::Function *func, ssa::BasicBlock &block);
    void copy_members(InsertionContext &ctx, Ref dst, Ref src, const ssa::Type &type);
    Ref create_member_pointer(InsertionContext &ctx, Ref ref, const ssa::Type &parent_type, unsigned index);
    void apply_splits(ssa::Function &func);

    StackValue *look_up_stack_value(ssa::VirtualRegister reg);
    std::optional<unsigned> look_up_stack_value_index(ssa::VirtualRegister reg);
    void disable_parent_splitting(StackValue &value);
    bool is_aggregate(const ssa::Type &type);

    void dump(ssa::Function &func);
    void dump_stack_values();
    void dump_stack_value(std::optional<ssa::VirtualRegister> reg, StackValue &value, unsigned indent);
    void dump_stack_replacements();
};

} // namespace passes

} // namespace banjo

#endif
