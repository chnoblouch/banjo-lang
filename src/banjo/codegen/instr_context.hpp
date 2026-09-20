#ifndef BANJO_CODEGEN_INSTR_CONTEXT_H
#define BANJO_CODEGEN_INSTR_CONTEXT_H

#include "banjo/mcode/basic_block.hpp"
#include "banjo/mcode/function.hpp"
#include "banjo/mcode/instruction.hpp"

namespace banjo::codegen {

struct InstrContext {
    mcode::Function *func;
    mcode::BasicBlockIter block;
    mcode::InstrIter instr;
};

} // namespace banjo::codegen

#endif
