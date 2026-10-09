#include "ssa_util.hpp"

#include "banjo/passes/control_flow_opt_pass.hpp"
#include "banjo/passes/inlining_pass.hpp"
#include "banjo/passes/peephole_optimizer.hpp"
#include "banjo/passes/sroa_pass.hpp"
#include "banjo/passes/stack_to_reg_pass.hpp"
#include "banjo/ssa/basic_block.hpp"
#include "banjo/ssa/ssa_parser.hpp"
#include "banjo/ssa/virtual_register.hpp"
#include "banjo/ssa/writer.hpp"
#include "banjo/target/target_description.hpp"
#include "banjo/utils/generic_lexer.hpp"
#include "banjo/utils/hash_map.hpp"
#include "banjo/utils/macros.hpp"

#include <iostream>
#include <string>

namespace banjo::test {

static void replace_regs(const HashMap<ssa::VirtualRegister, ssa::VirtualRegister> &reg_map, ssa::Operand &operand) {
    if (operand.is_register()) {
        operand.set_to_register(reg_map.find(operand.get_register()));
    } else if (operand.is_branch_target()) {
        for (ssa::Operand &arg : operand.get_branch_target().args) {
            replace_regs(reg_map, arg);
        }
    }
}

void renumber(ssa::Module &mod) {
    HashMap<ssa::VirtualRegister, ssa::VirtualRegister> reg_map;
    HashMap<ssa::BasicBlock *, unsigned> block_map;

    ssa::VirtualRegister next_reg = 0;
    unsigned next_block_id = 0;

    for (ssa::Function *func : mod.get_functions()) {
        for (ssa::BasicBlock &block : *func) {
            if (block.has_label()) {
                block_map.insert(&block, next_block_id);
                next_block_id += 1;
            }

            for (ssa::VirtualRegister &param_reg : block.param_regs) {
                reg_map.insert(param_reg, next_reg);
                next_reg += 1;
            }

            for (ssa::Instruction &instr : block) {
                if (instr.get_dest()) {
                    reg_map.insert(*instr.get_dest(), next_reg);
                    next_reg += 1;
                }
            }
        }
    }

    for (ssa::Function *func : mod.get_functions()) {
        for (ssa::BasicBlock &block : *func) {
            if (block.has_label()) {
                block.label = "b." + std::to_string(block_map.find(&block));
            }

            for (ssa::VirtualRegister &param_reg : block.param_regs) {
                param_reg = reg_map.find(param_reg);
            }

            for (ssa::Instruction &instr : block) {
                if (instr.get_dest()) {
                    instr.set_dest(reg_map.find(*instr.get_dest()));
                }

                for (ssa::Operand &operand : instr.get_operands()) {
                    replace_regs(reg_map, operand);
                }
            }
        }
    }
}

void SSAUtil::optimize(std::string_view pass_name, std::string_view source) {
    target::TargetDescription target_descr(
        target::Architecture::X86_64,
        target::OperatingSystem::LINUX,
        target::Environment::GNU
    );

    target::Target *target = target::Target::create(target_descr, target::CodeModel::LARGE);

    utils::TokenStream tokens = utils::GenericLexer{source}.tokenize();
    ssa::Module ssa_mod = ssa::Parser{tokens, target->get_default_calling_conv()}.parse();

    if (pass_name == "peephole") {
        passes::PeepholeOptimizer{target}.run(ssa_mod);
    } else if (pass_name == "sroa") {
        passes::SROAPass{target}.run(ssa_mod);
    } else if (pass_name == "stack_to_reg") {
        passes::StackToRegPass{target}.run(ssa_mod);
    } else if (pass_name == "inlining") {
        passes::InliningPass{target}.run(ssa_mod);
    } else if (pass_name == "control_flow_opt") {
        passes::ControlFlowOptPass{target}.run(ssa_mod);
    } else {
        ASSERT_UNREACHABLE;
    }

    renumber(ssa_mod);
    ssa::Writer{std::cout}.write(ssa_mod);

    delete target;
}

} // namespace banjo::test
