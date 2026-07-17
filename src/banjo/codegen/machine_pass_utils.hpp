#ifndef BANJO_CODEGEN_MACHINE_PASS_UTILS_H
#define BANJO_CODEGEN_MACHINE_PASS_UTILS_H

#include "banjo/mcode/function.hpp"

#include <vector>

namespace banjo {

namespace codegen {

namespace MachinePassUtils {

std::vector<long> get_modified_volatile_regs(mcode::Function *func);

} // namespace MachinePassUtils

} // namespace codegen

} // namespace banjo

#endif
