#include "validator.hpp"

#include "banjo/passes/pass_utils.hpp"

#include <set>

namespace banjo {

namespace ssa {

Validator::Validator(std::ostream &stream) : stream(stream) {}

bool Validator::validate(ssa::Module &mod) {
    bool valid = true;

    for (ssa::Function *func : mod.get_functions()) {
        valid = valid && validate(mod, *func);
    }

    return valid;
}

bool Validator::validate(ssa::Module &mod, ssa::Function &func) {
    bool valid = true;

    std::set<ssa::VirtualRegister> defs;

    for (ssa::BasicBlock &block : func) {
        for (ssa::VirtualRegister param_reg : block.get_param_regs()) {
            defs.insert(param_reg);
        }

        for (ssa::Instruction &instr : block) {
            if (instr.get_dest()) {
                defs.insert(*instr.get_dest());
            }
        }
    }

    for (ssa::BasicBlock &block : func) {
        for (ssa::Instruction &instr : block) {
            passes::PassUtils::iter_regs(instr.get_operands(), [&](ssa::VirtualRegister reg) {
                if (!defs.contains(reg)) {
                    stream << "error in `" << func.name << "`: %" << reg << " is not defined\n";
                    valid = false;
                }
            });

            if (instr.get_dest()) {
                defs.insert(*instr.get_dest());
            }

            switch (instr.get_opcode()) {
                case ssa::Opcode::MEMBERPTR: valid = valid && validate_memberptr(instr); break;
                default: break;
            }
        }
    }

    return valid;
}

bool Validator::validate_memberptr(ssa::Instruction &instr) {
    const ssa::Type &type = instr.get_operand(0).get_type();
    unsigned index = instr.get_operand(2).get_int_immediate().to_u64();

    if (type.is_struct()) {
        ssa::Structure &struct_ = *type.get_struct();
        if (index >= struct_.members.size()) {
            stream << "out of bounds memberptr";
            return false;
        }
    }

    return true;
}

} // namespace ssa

} // namespace banjo
