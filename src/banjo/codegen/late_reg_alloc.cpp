#include "late_reg_alloc.hpp"

#include <iostream>

namespace banjo {

namespace codegen {

LateRegAlloc::LateRegAlloc(Range range, RegClass reg_class, target::TargetRegAnalyzer &analyzer)
  : range(range),
    reg_class(reg_class),
    analyzer(analyzer) {}

std::optional<mcode::PhysicalReg> LateRegAlloc::alloc() {
    for (mcode::PhysicalReg candidate : analyzer.get_candidates(reg_class)) {
        if (check_alloc(candidate)) {
            return candidate;
        }
    }

    std::cerr << "register allocator is out of registers" << std::endl;
    std::cerr << "in function " << range.block.get_func()->get_name() << std::endl;
    std::exit(1);

    return -1;
}

bool LateRegAlloc::check_alloc(mcode::PhysicalReg reg) {
    for (mcode::InstrIter iter = range.start; iter != range.end.get_next(); ++iter) {
        if (analyzer.is_reg_overridden(*iter, range.block, reg)) {
            return false;
        }

        // If we encounter a call argument move, the register is reserved.
        if (iter->is_flag(mcode::Instruction::FLAG_CALL_ARG) && iter->get_dest().is_physical_reg()) {
            if (iter->get_dest().get_physical_reg() == reg) {
                return false;
            }
        }
    }

    for (mcode::InstrIter iter = range.end; iter != range.start.get_prev(); --iter) {
        // If we encounter a call argument move, the register is reserved.
        if (iter->is_flag(mcode::Instruction::FLAG_CALL_ARG) && iter->get_dest().is_physical_reg()) {
            if (iter->get_dest().get_physical_reg() == reg) {
                return false;
            }
        }

        // If we encounter a call instruction, all previously reserved call argument registers are free.
        if (iter->get_flags() == mcode::Instruction::FLAG_CALL) {
            break;
        }
    }

    return true;
}

} // namespace codegen

} // namespace banjo
