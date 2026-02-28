#ifndef BANJO_SIR_TO_TEXT_H
#define BANJO_SIR_TO_TEXT_H

#include "banjo/sir/sir.hpp"

#include <string_view>

namespace banjo::lang::sir {

std::string_view to_text(BinaryOp binary_op);

}

#endif
