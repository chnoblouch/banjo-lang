#ifndef CODEGEN_REG_ALLOC_FUNC_H
#define CODEGEN_REG_ALLOC_FUNC_H

#include "mcode/function.hpp"
#include <vector>

namespace codegen {

struct RegAllocInstr {
    unsigned index;
    mcode::InstrIter iter;
    std::vector<mcode::RegOp> regs;
    unsigned spill_tmp_regs = 0;
};

struct RegAllocPoint {
    unsigned instr;
    unsigned stage; // 0 = use, 1 = def
};

struct RegAllocRange {
    mcode::PhysicalReg reg;
    RegAllocPoint start;
    RegAllocPoint end;

    bool intersects(const RegAllocRange &other) {
        if (reg != other.reg) {
            return false;
        }

        unsigned startA = 2 * start.instr + start.stage;
        unsigned endA = 2 * end.instr + end.stage;
        unsigned startB = 2 * other.start.instr + other.start.stage;
        unsigned endB = 2 * other.end.instr + other.end.stage;
        return (startA <= endB) && (endA >= startB);
    }
};

struct RegAllocBlock {
    mcode::BasicBlockIter m_block;
    std::vector<RegAllocInstr> instrs;
    std::vector<unsigned> preds;
    std::vector<unsigned> succs;
    std::vector<RegAllocRange> allocated_ranges;
};

struct RegAllocFunc {
    mcode::Function &m_func;
    std::vector<RegAllocBlock> blocks;
};

} // namespace codegen

#endif
