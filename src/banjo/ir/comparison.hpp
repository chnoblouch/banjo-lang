#ifndef IR_COMPARISON_H
#define IR_COMPARISON_H

namespace ir {

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

} // namespace ir

#endif
