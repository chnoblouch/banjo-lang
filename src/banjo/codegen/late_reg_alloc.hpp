#ifndef BANJO_CODEGEN_LATE_REG_ALLOC_H
#define BANJO_CODEGEN_LATE_REG_ALLOC_H

#include "banjo/codegen/reg_alloc_func.hpp"
#include "banjo/mcode/basic_block.hpp"
#include "banjo/mcode/function.hpp"
#include "banjo/target/target_reg_analyzer.hpp"

#include <optional>

namespace banjo::codegen {

class LateRegAlloc {

public:
    struct Range {
        mcode::BasicBlockIter block;
        mcode::InstrIter start;
        mcode::InstrIter end;
    };

private:
    mcode::Function &func;
    Range range;
    RegClass reg_class;
    target::TargetRegAnalyzer &analyzer;

public:
    LateRegAlloc(mcode::Function &func, Range range, RegClass reg_class, target::TargetRegAnalyzer &analyzer);
    std::optional<mcode::PhysicalReg> alloc();

private:
    bool check_alloc(mcode::PhysicalReg reg);
};

} // namespace banjo::codegen

#endif
