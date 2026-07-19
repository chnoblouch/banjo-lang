#include "prolog_epilog_pass.hpp"

#include "banjo/mcode/calling_convention.hpp"
#include "banjo/utils/timing.hpp"

namespace banjo::codegen {

void PrologEpilogPass::run(mcode::Module &mod) {
    for (mcode::Function *func : mod.get_functions()) {
        run(*func);
    }
}

void PrologEpilogPass::run(mcode::Function &func) {
    PROFILE_SCOPE("prolog/epilog insertion");

    insert_prolog(func);
    insert_epilog(func);
}

void PrologEpilogPass::insert_prolog(mcode::Function &func) {
    mcode::BasicBlock &basic_block = func.get_basic_blocks().get_first();
    mcode::CallingConvention *calling_conv = func.get_calling_conv();

    mcode::InstrIter insertion_instr = basic_block.begin();
    for (mcode::Instruction &instr : calling_conv->get_prolog(&func)) {
        basic_block.insert_before(insertion_instr, instr);
    }
}

void PrologEpilogPass::insert_epilog(mcode::Function &func) {
    mcode::CallingConvention *calling_conv = func.get_calling_conv();

    for (mcode::BasicBlock &basic_block : func.get_basic_blocks()) {
        for (mcode::InstrIter iter = basic_block.begin(); iter != basic_block.end(); ++iter) {
            // Only insert epilogs before exit instructions.
            mcode::Opcode opcode = iter->get_opcode();
            if (!calling_conv->is_func_exit(opcode)) {
                continue;
            }

            for (mcode::Instruction &instr : calling_conv->get_epilog(&func)) {
                basic_block.insert_before(iter, instr);
            }
        }
    }
}

} // namespace banjo::codegen
