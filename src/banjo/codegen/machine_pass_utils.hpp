#ifndef BANJO_CODEGEN_MACHINE_PASS_UTILS_H
#define BANJO_CODEGEN_MACHINE_PASS_UTILS_H

#include "banjo/mcode/function.hpp"
#include "banjo/mcode/register.hpp"

#include <vector>

namespace banjo::codegen {

namespace MachinePassUtils {

std::vector<mcode::PhysicalReg> get_modified_volatile_regs(mcode::Function *func);

} // namespace MachinePassUtils

} // namespace banjo::codegen

#endif
