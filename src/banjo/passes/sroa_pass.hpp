#ifndef PASSES_SROA_PASS_H
#define PASSES_SROA_PASS_H

#include "passes/pass.hpp"

#include <functional>
#include <optional>

namespace passes {

class SROAPass : public Pass {

private:
    struct StackValue {
        ir::InstrIter alloca_instr;
        ir::BasicBlockIter alloca_block;
        std::vector<ir::InstrIter> uses;
        ir::Type type;
        unsigned parent;
        std::vector<unsigned> members;
        std::optional<ir::VirtualRegister> split_alloca;
        bool splittable = true;
    };

    struct InsertionContext {
        ir::Module &mod;
        ir::Function *func;
        ir::BasicBlock &block;
        ir::InstrIter instr;
    };

    struct Ref {
        ir::VirtualRegister ptr;
        std::optional<unsigned> stack_val;
    };

    typedef std::function<void(unsigned index, const ir::Type &type)> MemberAccessFunc;

    std::vector<StackValue> stack_values;
    std::unordered_map<ir::VirtualRegister, unsigned> roots;
    std::unordered_map<ir::VirtualRegister, unsigned> stack_ptr_defs;

public:
    SROAPass(target::Target *target);
    void run(ir::Module &mod);

private:
    void run(ir::Function *func, ir::Module &mod);
    void collect_stack_values(ir::BasicBlockIter block_iter);
    bool is_splitting_possible(const ir::Type &type);
    void collect_members(unsigned val_index);
    void collect_uses(ir::BasicBlock &block);
    void disable_splitting(StackValue &value);
    void analyze_memberptr(ir::InstrIter iter);
    void split_root(StackValue &value, ir::Function *func);
    void split_member(StackValue &value, ir::Function *func);
    void split_copies(ir::Function *func, ir::BasicBlock &block, ir::Module &mod);
    std::optional<unsigned> find_stack_val(ir::VirtualRegister reg);
    void copy_members(InsertionContext &ctx, Ref dst, Ref src, const ir::Type &type);
    Ref get_final_memberptr(InsertionContext &ctx, Ref ref, const ir::Type &parent_type, unsigned index);
    void apply_splits(ir::BasicBlock &block);
    bool is_aggregate(const ir::Type &type);
    void for_each_member(const ir::Type &type, const MemberAccessFunc &func);
    void dump_stack_values();
    void dump_stack_value(StackValue &value, unsigned indent);
};

} // namespace passes

#endif
