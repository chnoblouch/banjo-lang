#include "machine_pass_utils.hpp"

#include "banjo/mcode/calling_convention.hpp"

#include <algorithm>

namespace banjo::codegen {

std::vector<mcode::PhysicalReg> MachinePassUtils::get_modified_volatile_regs(mcode::Function *func) {
    std::vector<mcode::PhysicalReg> result;

    for (mcode::BasicBlock &block : func->get_basic_blocks()) {
        for (mcode::Instruction &instr : block) {
            if (instr.has_dest() && instr.get_dest().is_physical_reg() &&
                !func->get_calling_conv()->is_volatile(instr.get_dest().get_physical_reg()) &&
                std::find(result.begin(), result.end(), instr.get_dest().get_physical_reg()) == result.end()) {
                result.push_back(instr.get_dest().get_physical_reg());
            }
        }
    }

    return result;
}

} // namespace banjo::codegen
