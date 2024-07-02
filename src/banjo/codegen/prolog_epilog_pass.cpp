#include "prolog_epilog_pass.hpp"

#include "banjo/codegen/machine_pass_utils.hpp"
#include "banjo/utils/timing.hpp"

#include <vector>

namespace banjo {

namespace codegen {

void PrologEpilogPass::run(mcode::Module &module_) {
    for (mcode::Function *func : module_.get_functions()) {
        run(func);
    }
}

void PrologEpilogPass::run(mcode::Function *func) {
    PROFILE_SCOPE("prolog/epilog insertion");

    std::vector<long> modified_volatile_regs = MachinePassUtils::get_modified_volatile_regs(func);

    insert_prolog(func, modified_volatile_regs);
    insert_epilog(func, modified_volatile_regs);
}

void PrologEpilogPass::insert_prolog(mcode::Function *func, std::vector<long> &modified_volatile_regs) {
    mcode::BasicBlock &basic_block = func->get_basic_blocks().get_first();
    mcode::CallingConvention *calling_conv = func->get_calling_conv();

    mcode::InstrIter insertion_instr = basic_block.begin();
    for (mcode::Instruction &instr : calling_conv->get_prolog(func)) {
        basic_block.insert_before(insertion_instr, instr);
    }
}

void PrologEpilogPass::insert_epilog(mcode::Function *func, std::vector<long> &modified_volatile_regs) {
    mcode::CallingConvention *calling_conv = func->get_calling_conv();

    for (mcode::BasicBlock &basic_block : func->get_basic_blocks()) {
        for (mcode::InstrIter iter = basic_block.begin(); iter != basic_block.end(); ++iter) {
            // Only insert epilogs before exit instructions.
            mcode::Opcode opcode = iter->get_opcode();
            if (!calling_conv->is_func_exit(opcode)) {
                continue;
            }

            for (mcode::Instruction &instr : calling_conv->get_epilog(func)) {
                basic_block.insert_before(iter, instr);
            }
        }
    }
}

} // namespace codegen

} // namespace banjo
