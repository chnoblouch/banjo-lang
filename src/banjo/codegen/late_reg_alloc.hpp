#ifndef CODEGEN_LATE_REG_ALLOC_H
#define CODEGEN_LATE_REG_ALLOC_H

#include "banjo/codegen/reg_alloc_func.hpp"
#include "banjo/mcode/basic_block.hpp"
#include "banjo/target/target_reg_analyzer.hpp"

#include <optional>

namespace banjo {

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

} // namespace banjo

#endif
