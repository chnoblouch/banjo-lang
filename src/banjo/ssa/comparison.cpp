#include "banjo/ssa/comparison.hpp"

namespace banjo::ssa {

Comparison invert_comparison(Comparison comparison) {
    switch (comparison) {
        case ssa::Comparison::EQ: return ssa::Comparison::NE;   // !(a == b) == (a != b)
        case ssa::Comparison::NE: return ssa::Comparison::EQ;   // !(a != b) == (a == b)
        case ssa::Comparison::UGT: return ssa::Comparison::ULE; // !(a >  b) == (a <= b)
        case ssa::Comparison::UGE: return ssa::Comparison::ULT; // !(a >= b) == (a <  b)
        case ssa::Comparison::ULT: return ssa::Comparison::UGE; // !(a <  b) == (a >= b)
        case ssa::Comparison::ULE: return ssa::Comparison::UGT; // !(a <= b) == (a >  b)
        case ssa::Comparison::SGT: return ssa::Comparison::SLE; // !(a >  b) == (a <= b)
        case ssa::Comparison::SGE: return ssa::Comparison::SLT; // !(a >= b) == (a <  b)
        case ssa::Comparison::SLT: return ssa::Comparison::SGE; // !(a <  b) == (a >= b)
        case ssa::Comparison::SLE: return ssa::Comparison::SGT; // !(a <= b) == (a >  b)
        case ssa::Comparison::FEQ: return ssa::Comparison::FNE; // !(a == b) == (a != b)
        case ssa::Comparison::FNE: return ssa::Comparison::FEQ; // !(a != b) == (a == b)
        case ssa::Comparison::FGT: return ssa::Comparison::FLE; // !(a >  b) == (a <= b)
        case ssa::Comparison::FGE: return ssa::Comparison::FLT; // !(a >= b) == (a <  b)
        case ssa::Comparison::FLT: return ssa::Comparison::FGE; // !(a <  b) == (a >= b)
        case ssa::Comparison::FLE: return ssa::Comparison::FGT; // !(a <= b) == (a >  b)
    }
}

Comparison comparison_swapped(Comparison comparison) {
    switch (comparison) {
        case Comparison::EQ: return Comparison::EQ;   // (a == b) == (b == a)
        case Comparison::NE: return Comparison::NE;   // (a != b) == (b != a)
        case Comparison::UGT: return Comparison::ULT; // (a >  b) == (b <  a)
        case Comparison::UGE: return Comparison::ULE; // (a >= b) == (b <= a)
        case Comparison::ULT: return Comparison::UGT; // (a <  b) == (b >  a)
        case Comparison::ULE: return Comparison::UGE; // (a <= b) == (b >= a)
        case Comparison::SGT: return Comparison::SLT; // (a >  b) == (b <  a)
        case Comparison::SGE: return Comparison::SLE; // (a >= b) == (b <= a)
        case Comparison::SLT: return Comparison::SGT; // (a <  b) == (b >  a)
        case Comparison::SLE: return Comparison::SGE; // (a <= b) == (b >= a)
        case Comparison::FEQ: return Comparison::FEQ; // (a == b) == (b == a)
        case Comparison::FNE: return Comparison::FNE; // (a != b) == (b != a)
        case Comparison::FGT: return Comparison::FLT; // (a >  b) == (b <  a)
        case Comparison::FGE: return Comparison::FLE; // (a >= b) == (b <= a)
        case Comparison::FLT: return Comparison::FGT; // (a <  b) == (b >  a)
        case Comparison::FLE: return Comparison::FGE; // (a <= b) == (b >= a)
    }
}

bool is_comparison_signed(Comparison comparison) {
    switch (comparison) {
        case ssa::Comparison::SGT:
        case ssa::Comparison::SGE:
        case ssa::Comparison::SLT:
        case ssa::Comparison::SLE: return true;
        default: return false;
    }
}

} // namespace banjo::ssa
