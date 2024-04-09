#ifndef CODEGEN_REG_ALLOC_PASS_H
#define CODEGEN_REG_ALLOC_PASS_H

#define DEBUG_REG_ALLOC 0

#include "codegen/liveness.hpp"
#include "codegen/machine_pass.hpp"
#include "codegen/reg_alloc_func.hpp"
#include "target/target_reg_analyzer.hpp"

#include <fstream>
#include <queue>
#include <unordered_set>
#include <vector>

namespace codegen {

class RegAllocPass : public MachinePass {

private:
    struct GroupComparator {
        bool operator()(const LiveRangeGroup &lhs, const LiveRangeGroup &rhs);
        unsigned get_weight(const LiveRangeGroup &group);
    };

    typedef std::priority_queue<LiveRangeGroup, std::vector<LiveRangeGroup>, GroupComparator> Queue;

    struct Alloc {
        bool is_physical_reg;
        union {
            mcode::PhysicalReg physical_reg;
            long stack_slot;
        };

        LiveRangeGroup group;
    };

    struct Context {
        RegAllocFunc &func;
        LivenessAnalysis &liveness;
        mcode::BasicBlockIter block_iter;
        std::unordered_map<mcode::VirtualReg, Alloc> reg_map;
    };

    target::TargetRegAnalyzer &analyzer;

#if DEBUG_REG_ALLOC
    std::ofstream stream{"liveness.txt"};
#endif

public:
    RegAllocPass(target::TargetRegAnalyzer &analyzer);
    void run(mcode::Module &module_);
    void run(mcode::Function &func);

private:
    RegAllocFunc create_reg_alloc_func(mcode::Function &func);
    std::vector<RegAllocInstr> collect_instrs(mcode::BasicBlock &basic_block);
    void reserve_range(Context &ctx, LiveRangeGroup &group, mcode::PhysicalReg reg);
    Alloc alloc_group(Context &ctx, LiveRangeGroup &group);
    bool is_alloc_possible(Context &ctx, LiveRangeGroup &group, mcode::PhysicalReg reg);
    void replace_with_physical_reg(Context &ctx, Alloc &alloc);
    void try_replace(mcode::Operand &operand, mcode::VirtualReg virtual_reg, mcode::PhysicalReg physical_reg);
    void insert_spilled_load_store(Context &ctx, mcode::VirtualReg virtual_reg, Alloc &alloc, RegAllocInstr &instr);
    void remove_useless_instrs(mcode::BasicBlock &basic_block);
};

} // namespace codegen

#endif
