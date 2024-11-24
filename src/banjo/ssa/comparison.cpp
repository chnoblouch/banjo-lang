#include "banjo/ssa/comparison.hpp"

namespace banjo {

namespace ssa {

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

} // namespace ssa

} // namespace banjo
