#include "canonicalization_pass.hpp"

#include "banjo/passes/pass_utils.hpp"
#include "banjo/target/target_data_layout.hpp"

namespace banjo::passes {

CanonicalizationPass::CanonicalizationPass(target::Target *target) : Pass("canonicalization", target) {}

void CanonicalizationPass::run(ssa::Module &mod) {
    for (ssa::Function *func : mod.get_functions()) {
        run(*func);
    }
}

void CanonicalizationPass::run(ssa::Function &func) {
    for (ssa::BasicBlock &block : func.get_basic_blocks()) {
        run(func, block);
    }
}

void CanonicalizationPass::run(ssa::Function &func, ssa::BasicBlock &block) {
    // TODO:
    //   - Merge `offsetptr` instructions
    //   - Handle `offsetptr`s into nested structures

    target::TargetDataLayout &data_layout = get_target()->get_data_layout();

    for (ssa::Instruction &instr : block) {
        if (instr.get_opcode() != ssa::Opcode::OFFSETPTR) {
            continue;
        }

        ssa::Operand &base = instr.get_operand(0);
        ssa::Operand &offset = instr.get_operand(1);
        ssa::Type type = instr.get_operand(2).get_type();

        if (!base.is_register() || !offset.is_int_immediate()) {
            continue;
        }

        ssa::InstrIter base_def = PassUtils::find_def(func, base.get_register());
        ssa::Type base_type = base_def->get_type();

        if (!base_type.is_struct() || base_type.get_array_length() != 1) {
            continue;
        }

        ssa::Structure *struct_ = base_type.get_struct();
        unsigned byte_offset = offset.get_int_immediate().to_unsigned() * data_layout.get_size(type);

        for (unsigned i = 0; i < struct_->members.size(); i++) {
            unsigned member_offset = data_layout.get_member_offset(struct_, i);

            if (member_offset == byte_offset) {
                ssa::Operand base_type_operand = ssa::Operand::from_type(base_type);
                ssa::Operand member_index_operand = ssa::Operand::from_int_immediate(i);
                instr = {ssa::Opcode::MEMBERPTR, instr.get_dest(), {base_type_operand, base, member_index_operand}};
                break;
            } else if (member_offset > byte_offset) {
                break;
            }
        }
    }
}

} // namespace banjo::passes
