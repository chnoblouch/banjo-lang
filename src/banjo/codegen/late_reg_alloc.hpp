#ifndef CODEGEN_LATE_REG_ALLOC_H
#define CODEGEN_LATE_REG_ALLOC_H

#include "codegen/reg_alloc_func.hpp"
#include "mcode/basic_block.hpp"
#include "target/target_reg_analyzer.hpp"

#include <optional>

namespace codegen {

class LateRegAlloc {

public:
    struct Range {
        mcode::BasicBlock &block;
        mcode::InstrIter start;
        mcode::InstrIter end;
    };

private:
    Range range;
    RegClass reg_class;
    target::TargetRegAnalyzer &analyzer;

public:
    LateRegAlloc(Range range, RegClass reg_class, target::TargetRegAnalyzer &analyzer);
    std::optional<mcode::PhysicalReg> alloc();

private:
    bool check_alloc(mcode::PhysicalReg reg);
};

} // namespace codegen

#endif
