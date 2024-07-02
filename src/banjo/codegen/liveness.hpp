#ifndef CODEGEN_LIVENESS_H
#define CODEGEN_LIVENESS_H

#include "banjo/codegen/reg_alloc_func.hpp"
#include "banjo/mcode/register.hpp"

#include <ostream>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace banjo {

namespace codegen {

struct BlockLiveness {
    std::unordered_set<mcode::Register> defs;
    std::unordered_set<mcode::Register> uses;
    std::unordered_set<mcode::Register> ins;
    std::unordered_set<mcode::Register> outs;
};

struct KillPoint {
    mcode::PhysicalReg reg;
    unsigned block;
    unsigned instr;
};

class LivenessAnalysis {

public:
    RegAllocFunc &func;
    std::vector<BlockLiveness> block_liveness;
    std::unordered_map<mcode::Register, std::vector<LiveRange>> reg_ranges;
    std::vector<KillPoint> kill_points;

private:
    LivenessAnalysis(RegAllocFunc &func);

public:
    static LivenessAnalysis compute(RegAllocFunc &func);
    void dump(std::ostream &stream);

private:
    static void collect_uses_and_defs(RegAllocBlock &block, BlockLiveness &liveness);
    static void compute_ins_and_outs(RegAllocFunc &func, LivenessAnalysis &analysis);

    static void collect_blocks_post_order(
        RegAllocFunc &func,
        unsigned block_index,
        std::vector<unsigned> &indices,
        std::unordered_set<unsigned> &blocks_visited
    );

    static void compute_precise_live_ranges(RegAllocFunc &func, LivenessAnalysis &analysis);
};

} // namespace codegen

} // namespace banjo

#endif
