#include "banjo/ssa/comparison.hpp"

namespace banjo::ssa {

Comparison invert_comparison(Comparison comparison) {
    switch (comparison) {
        case ssa::Comparison::EQ: return ssa::Comparison::NE;
        case ssa::Comparison::NE: return ssa::Comparison::EQ;
        case ssa::Comparison::UGT: return ssa::Comparison::ULE;
        case ssa::Comparison::UGE: return ssa::Comparison::ULT;
        case ssa::Comparison::ULT: return ssa::Comparison::UGE;
        case ssa::Comparison::ULE: return ssa::Comparison::UGT;
        case ssa::Comparison::SGT: return ssa::Comparison::SLE;
        case ssa::Comparison::SGE: return ssa::Comparison::SLT;
        case ssa::Comparison::SLT: return ssa::Comparison::SGE;
        case ssa::Comparison::SLE: return ssa::Comparison::SGT;
        case ssa::Comparison::FEQ: return ssa::Comparison::FNE;
        case ssa::Comparison::FNE: return ssa::Comparison::FEQ;
        case ssa::Comparison::FGT: return ssa::Comparison::FLE;
        case ssa::Comparison::FGE: return ssa::Comparison::FLT;
        case ssa::Comparison::FLT: return ssa::Comparison::FGE;
        case ssa::Comparison::FLE: return ssa::Comparison::FGT;
    }
}

Comparison comparison_swapped(Comparison comparison) {
    switch (comparison) {
        case Comparison::EQ: return Comparison::EQ;
        case Comparison::NE: return Comparison::NE;
        case Comparison::UGT: return Comparison::ULE;
        case Comparison::UGE: return Comparison::ULT;
        case Comparison::ULT: return Comparison::UGE;
        case Comparison::ULE: return Comparison::UGT;
        case Comparison::SGT: return Comparison::SLE;
        case Comparison::SGE: return Comparison::SLT;
        case Comparison::SLT: return Comparison::SGE;
        case Comparison::SLE: return Comparison::SGT;
        case Comparison::FEQ: return Comparison::FEQ;
        case Comparison::FNE: return Comparison::FNE;
        case Comparison::FGT: return Comparison::FLE;
        case Comparison::FGE: return Comparison::FLT;
        case Comparison::FLT: return Comparison::FGE;
        case Comparison::FLE: return Comparison::FGT;
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
