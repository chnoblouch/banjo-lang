#ifndef CODEGEN_REG_ALLOC_FUNC_H
#define CODEGEN_REG_ALLOC_FUNC_H

#include "banjo/mcode/function.hpp"
#include "banjo/mcode/register.hpp"

#include <unordered_map>
#include <vector>

namespace banjo {

namespace codegen {

struct RegAllocInstr {
    unsigned index;
    mcode::InstrIter iter;
    std::vector<mcode::RegOp> regs;
};

struct RegAllocPoint {
    unsigned instr;
    unsigned stage; // 0 = use, 1 = def
};

struct LiveRange {
    unsigned block;
    RegAllocPoint start;
    RegAllocPoint end;

    bool intersects(const LiveRange &other) const {
        if (block != other.block) {
            return false;
        }

        unsigned startA = 2 * start.instr + start.stage;
        unsigned endA = 2 * end.instr + end.stage;
        unsigned startB = 2 * other.start.instr + other.start.stage;
        unsigned endB = 2 * other.end.instr + other.end.stage;
        return (startA <= endB) && (endA >= startB);
    }
};

struct ReservedSegment {
    mcode::PhysicalReg reg;
    LiveRange range;
};

struct RegAllocBlock {
    mcode::BasicBlockIter m_block;
    std::vector<RegAllocInstr> instrs;
    std::vector<unsigned> preds;
    std::vector<unsigned> succs;
    std::vector<ReservedSegment> reserved_segments;
};

struct RegAllocFunc {
    mcode::Function &m_func;
    std::vector<RegAllocBlock> blocks;
};

typedef unsigned RegClass;
typedef std::unordered_map<mcode::VirtualReg, RegClass> RegClassMap;

struct Segment {
    mcode::VirtualReg reg;
    LiveRange range;
};

struct Bundle {
    std::vector<Segment> segments;
    RegClass reg_class;
    bool evictable = true;
    std::optional<mcode::StackSlotID> src_stack_slot = {};
    std::optional<mcode::StackSlotID> dst_stack_slot = {};
    bool deleted = false;
};

} // namespace codegen

} // namespace banjo

#endif
