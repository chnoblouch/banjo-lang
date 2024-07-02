#include "ir/comparison.hpp"

namespace banjo {

namespace ir {

Comparison invert_comparison(Comparison comparison) {
    switch (comparison) {
        case ir::Comparison::EQ: return ir::Comparison::NE;
        case ir::Comparison::NE: return ir::Comparison::EQ;
        case ir::Comparison::UGT: return ir::Comparison::ULE;
        case ir::Comparison::UGE: return ir::Comparison::ULT;
        case ir::Comparison::ULT: return ir::Comparison::UGE;
        case ir::Comparison::ULE: return ir::Comparison::UGT;
        case ir::Comparison::SGT: return ir::Comparison::SLE;
        case ir::Comparison::SGE: return ir::Comparison::SLT;
        case ir::Comparison::SLT: return ir::Comparison::SGE;
        case ir::Comparison::SLE: return ir::Comparison::SGT;
        case ir::Comparison::FEQ: return ir::Comparison::FNE;
        case ir::Comparison::FNE: return ir::Comparison::FEQ;
        case ir::Comparison::FGT: return ir::Comparison::FLE;
        case ir::Comparison::FGE: return ir::Comparison::FLT;
        case ir::Comparison::FLT: return ir::Comparison::FGE;
        case ir::Comparison::FLE: return ir::Comparison::FGT;
    }
}

} // namespace ir

} // namespace banjo
