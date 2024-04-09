#include "late_reg_alloc.hpp"

#include <iostream>

namespace codegen {

LateRegAlloc::LateRegAlloc(mcode::BasicBlock &basic_block, Range range, target::TargetRegAnalyzer &analyzer)
  : basic_block(basic_block),
    range(range),
    analyzer(analyzer) {}

std::optional<int> LateRegAlloc::alloc() {
    mcode::Instruction &producer = *range.start;

    for (int candidate : analyzer.get_candidates(producer)) {
        if (check_alloc(candidate)) {
            return candidate;
        }
    }

    std::cerr << "register allocator is out of registers" << std::endl;
    std::cerr << "in function " << basic_block.get_func()->get_name() << std::endl;
    std::exit(1);

    return -1;
}

bool LateRegAlloc::check_alloc(int reg) {
    for (mcode::InstrIter iter = range.start; iter != range.end.get_next(); ++iter) {
        if (analyzer.is_reg_overridden(*iter, basic_block, reg)) {
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
