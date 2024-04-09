#include "validator.hpp"

#include "passes/pass_utils.hpp"

#include <set>

namespace ir {

Validator::Validator(std::ostream &stream) : stream(stream) {}

bool Validator::validate(ir::Module &mod) {
    bool valid = true;

    for (ir::Function *func : mod.get_functions()) {
        valid = valid && validate(mod, *func);
    }

    return valid;
}

bool Validator::validate(ir::Module &mod, ir::Function &func) {
    bool valid = true;

    std::set<ir::VirtualRegister> defs;

    for (ir::BasicBlock &block : func) {
        for (ir::VirtualRegister param_reg : block.get_param_regs()) {
            defs.insert(param_reg);
        }

        for (ir::Instruction &instr : block) {
            if (instr.get_dest()) {
                defs.insert(*instr.get_dest());
            }
        }
    }

    for (ir::BasicBlock &block : func) {
        for (ir::Instruction &instr : block) {
            passes::PassUtils::iter_regs(instr.get_operands(), [&defs, &valid, this](ir::VirtualRegister reg) {
                if (!defs.contains(reg)) {
                    stream << "%" << reg << " is not defined\n";
                    valid = false;
                }
            });

            if (instr.get_dest()) {
                defs.insert(*instr.get_dest());
            }

            switch (instr.get_opcode()) {
                case ir::Opcode::MEMBERPTR: valid = valid && validate_memberptr(mod, instr); break;
                default: break;
            }
        }
    }

    return valid;
}

bool Validator::validate_memberptr(ir::Module &mod, ir::Instruction &instr) {
    const ir::Type &type = instr.get_operand(0).get_type();
    unsigned index = instr.get_operand(1).get_int_immediate().to_u64();

    if (type.is_base_only() && type.is_struct()) {
        ir::Structure &struct_ = *type.get_struct();
        if (index >= struct_.get_members().size()) {
            stream << "out of bounds memberptr";
            return false;
        }
    }

    return true;
}

} // namespace ir
