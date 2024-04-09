#ifndef CODEGEN_LATE_REG_ALLOC_H
#define CODEGEN_LATE_REG_ALLOC_H

#include "mcode/basic_block.hpp"
#include "target/target_reg_analyzer.hpp"

#include <optional>

namespace codegen {

class LateRegAlloc {

public:
    struct Range {
        mcode::InstrIter start;
        mcode::InstrIter end;
    };

private:
    mcode::BasicBlock &basic_block;
    Range range;
    target::TargetRegAnalyzer &analyzer;

public:
    LateRegAlloc(mcode::BasicBlock &basic_block, Range range, target::TargetRegAnalyzer &analyzer);
    std::optional<int> alloc();

private:
    bool check_alloc(int reg);
};

} // namespace codegen

#endif
