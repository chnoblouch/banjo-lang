#ifndef BANJO_SSA_COMPARISON_H
#define BANJO_SSA_COMPARISON_H

namespace banjo::ssa {

enum class Comparison {
    EQ,
    NE,
    UGT,
    UGE,
    ULT,
    ULE,
    SGT,
    SGE,
    SLT,
    SLE,
    FEQ,
    FNE,
    FGT,
    FGE,
    FLT,
    FLE,
};

Comparison invert_comparison(Comparison comparison);
bool is_comparison_signed(Comparison comparison);

} // namespace banjo::ssa

#endif
