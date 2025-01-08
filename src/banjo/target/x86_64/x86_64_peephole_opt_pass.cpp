#include "x86_64_peephole_opt_pass.hpp"

#include "banjo/codegen/machine_pass_utils.hpp"
#include "banjo/target/x86_64/x86_64_opcode.hpp"
#include "banjo/utils/timing.hpp"
#include <unordered_set>

namespace banjo {

namespace target {

const std::unordered_set<mcode::Opcode> STACK_LOADING_OPS = {
    X8664Opcode::ADD,
    X8664Opcode::SUB,
    X8664Opcode::IMUL,
    X8664Opcode::CMP,
    X8664Opcode::ADDSS,
    X8664Opcode::SUBSS,
    X8664Opcode::MULSS,
    X8664Opcode::DIVSS,
    X8664Opcode::ADDSD,
    X8664Opcode::SUBSD,
    X8664Opcode::MULSD,
    X8664Opcode::DIVSD,
};

void X8664PeepholeOptPass::run(mcode::Module &module_) {
    for (mcode::Function *func : module_.get_functions()) {
        run(func);
    }
}

void X8664PeepholeOptPass::run(mcode::Function *func) {
    PROFILE_SCOPE("x86-64 peephole opt");

    for (mcode::BasicBlock &basic_block : func->get_basic_blocks()) {
        for (mcode::Instruction &instr : basic_block) {
            if (instr.get_opcode() == X8664Opcode::MOVSS && instr.get_operand(0).is_register() &&
                instr.get_operand(1).is_register()) {
                instr.set_opcode(X8664Opcode::MOVAPS);
            }
        }
    }
}

} // namespace target

} // namespace banjo
