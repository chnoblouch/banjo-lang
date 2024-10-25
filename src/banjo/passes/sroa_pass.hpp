#ifndef PASSES_SROA_PASS_H
#define PASSES_SROA_PASS_H

#include "banjo/ir/virtual_register.hpp"
#include "banjo/passes/pass.hpp"
#include "banjo/passes/pass_utils.hpp"

#include <functional>
#include <optional>

namespace banjo {

namespace passes {

class SROAPass : public Pass {

private:
    struct StackValue {
        ir::InstrIter alloca_instr;
        ir::BasicBlockIter alloca_block;
        ir::Type type;
        std::optional<ir::VirtualRegister> parent;
        std::optional<std::vector<unsigned>> members;
        std::optional<ir::VirtualRegister> replacement;
    };

    struct InsertionContext {
        ir::Function *func;
        ir::BasicBlock &block;
        ir::InstrIter instr;
    };

    struct Ref {
        std::optional<ir::VirtualRegister> ptr;
        std::optional<unsigned> stack_value;
    };

    std::vector<StackValue> stack_values;
    std::unordered_map<ir::VirtualRegister, unsigned> stack_ptr_defs;

public:
    SROAPass(target::Target *target);
    void run(ir::Module &mod);

private:
    void run(ir::Function *func);
    void collect_stack_values(ir::BasicBlockIter block_iter);
    void collect_members(unsigned val_index);
    void disable_invalid_splits(ir::BasicBlock &block);
    void split_root(StackValue &value, ir::Function *func);
    void split_member(StackValue &value, ir::Function *func);

    void collect_stack_ptr_defs(ir::Function &func);
    void collect_member_ptr_defs(PassUtils::UseMap &use_map, ir::VirtualRegister base, StackValue &value);

    void split_copies(ir::Function *func, ir::BasicBlock &block);
    void copy_members(InsertionContext &ctx, Ref dst, Ref src, const ir::Type &type);
    Ref create_member_pointer(InsertionContext &ctx, Ref ref, const ir::Type &parent_type, unsigned index);
    void apply_splits(ir::Function &func);

    StackValue *look_up_stack_value(ir::VirtualRegister reg);
    std::optional<unsigned> look_up_stack_value_index(ir::VirtualRegister reg);
    bool is_aggregate(const ir::Type &type);

    void dump(ir::Function &func);
    void dump_stack_values();
    void dump_stack_value(std::optional<ir::VirtualRegister> reg, StackValue &value, unsigned indent);
    void dump_stack_replacements();
};

} // namespace passes

} // namespace banjo

#endif
