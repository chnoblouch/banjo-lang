#include "ssa_util.hpp"

#include "banjo/passes/pass_utils.hpp"
#include "banjo/passes/stack_to_reg_pass.hpp"
#include "banjo/ssa/virtual_register.hpp"
#include "banjo/ssa/writer.hpp"
#include "banjo/target/target_description.hpp"

#include "ssa_parser.hpp"

namespace banjo {
namespace test {

void SSAUtil::optimize() {
    target::TargetDescription target_descr(
        target::Architecture::X86_64,
        target::OperatingSystem::LINUX,
        target::Environment::GNU
    );

    target::Target *target = target::Target::create(target_descr, target::CodeModel::LARGE);

    ssa::Module ssa_mod = SSAParser().parse();
    passes::StackToRegPass(target).run(ssa_mod);
    renumber_regs(ssa_mod);
    ssa::Writer(std::cout).write(ssa_mod);

    delete target;
}

void SSAUtil::renumber_regs(ssa::Module &mod) {
    RegMap reg_map;
    ssa::VirtualRegister next_reg = 0;

    for (ssa::Function *func : mod.get_functions()) {
        for (ssa::BasicBlock &block : *func) {
            for (ssa::Instruction &instr : block) {
                if (instr.get_dest()) {
                    reg_map.insert({*instr.get_dest(), next_reg});
                    next_reg += 1;
                }
            }
        }
    }

    for (ssa::Function *func : mod.get_functions()) {
        for (ssa::BasicBlock &block : *func) {
            for (ssa::Instruction &instr : block) {
                if (instr.get_dest()) {
                    instr.set_dest(reg_map.at(*instr.get_dest()));
                }

                for (ssa::Operand &operand : instr.get_operands()) {
                    replace_regs(reg_map, operand);
                }
            }
        }
    }
}

void SSAUtil::replace_regs(const RegMap &reg_map, ssa::Operand &operand) {
    if (operand.is_register()) {
        operand.set_to_register(reg_map.at(operand.get_register()));
    } else if (operand.is_branch_target()) {
        for (ssa::Operand &arg : operand.get_branch_target().args) {
            replace_regs(reg_map, arg);
        }
    }
}

} // namespace test
} // namespace banjo
