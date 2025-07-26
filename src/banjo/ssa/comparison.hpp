#ifndef BANJO_SSA_COMPARISON_H
#define BANJO_SSA_COMPARISON_H

namespace banjo {

namespace ssa {

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

} // namespace ssa

} // namespace banjo

#endif
