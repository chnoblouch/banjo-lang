#ifndef BANJO_CODEGEN_REG_ALLOC_PASS_H
#define BANJO_CODEGEN_REG_ALLOC_PASS_H

#include "banjo/codegen/liveness.hpp"
#include "banjo/codegen/machine_pass.hpp"
#include "banjo/codegen/reg_alloc_func.hpp"
#include "banjo/mcode/register.hpp"
#include "banjo/target/target.hpp"
#include "banjo/target/target_reg_analyzer.hpp"

#include <fstream>
#include <optional>
#include <queue>
#include <vector>

namespace banjo::codegen {

class RegAllocPass : public MachinePass {

private:
    struct BundleComparator {
        bool operator()(const Bundle &lhs, const Bundle &rhs);
        unsigned get_weight(const Bundle &bundle);
    };

    typedef std::priority_queue<Bundle, std::vector<Bundle>, BundleComparator> Queue;

    struct Alloc {
        mcode::PhysicalReg physical_reg{};
        Bundle bundle;
    };

    struct Context {
        RegAllocFunc &func;
        LivenessAnalysis &liveness;
        mcode::BasicBlockIter block_iter;
        RegClassMap reg_classes;
        Queue bundles;
        std::vector<Alloc> allocs;
    };

    target::Target &target;
    target::TargetRegAnalyzer &analyzer;

    std::vector<mcode::PhysicalReg> suggested_regs;
    std::optional<std::ofstream> stream;

public:
    RegAllocPass(target::Target &target, target::TargetRegAnalyzer &analyzer);
    void run(mcode::Module &mod);
    void run(mcode::Function &func);

private:
    RegAllocFunc create_reg_alloc_func(mcode::Function &func);
    std::vector<RegAllocInstr> collect_instrs(mcode::Function &func, mcode::BasicBlockIter block);
    void assign_reg_classes(Context &ctx);
    void reserve_fixed_ranges(Context &ctx);

    std::vector<Bundle> create_bundles(Context &ctx);
    std::vector<Bundle> coalesce_bundles(Context &ctx, std::vector<Bundle> bundles);
    void try_coalesce(Context &ctx, Bundle &a, Bundle &b);
    bool is_connecting_move(RegAllocBlock &block, const Segment &dst, const Segment &src);
    bool intersect(const Bundle &a, const Bundle &b);

    void alloc_bundle(Context &ctx, Bundle &bundle);
    bool try_assign_suggested_regs(Context &ctx, Bundle &bundle);
    bool try_assign_candidate_regs(Context &ctx, Bundle &bundle);
    bool try_alloc_physical_reg(Context &ctx, Bundle &bundle, mcode::PhysicalReg reg);
    bool is_alloc_possible(Context &ctx, Bundle &bundle, mcode::PhysicalReg reg);
    bool try_evict(Context &ctx, Bundle &bundle);
    bool can_evict(Bundle &bundle, Alloc &alloc);
    void spill(Context &ctx, Bundle &bundle);

    void apply_alloc(Context &ctx, const Alloc &alloc);
    void try_replace(mcode::Operand &operand, mcode::VirtualReg virtual_reg, mcode::PhysicalReg physical_reg);
    void remove_useless_instrs(mcode::BasicBlock &basic_block);

    void write_debug_report(Context &ctx);
};

} // namespace banjo::codegen

#endif
