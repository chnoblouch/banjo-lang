#include "x86_64_const_lowering.hpp"

#include "passes/pass_utils.hpp"
#include "target/x86_64/x86_64_opcode.hpp"

#include <cassert>

namespace banjo {

namespace target {

X8664ConstLowering::X8664ConstLowering(codegen::IRLowerer &lowerer) : lowerer(lowerer) {}

mcode::Value X8664ConstLowering::load_f32(float value) {
    assert(value != 0.0);

    if (lowerer.get_basic_block_iter() != last_block) {
        last_block = lowerer.get_basic_block_iter();
        f32_storage.clear();
        process_block();
    }

    ConstStorage storage = f32_storage.at(lowerer.get_instr_iter()).at(value);

    if (storage.access == ConstStorageAccess::LOAD) {
        return mcode::Operand::from_symbol_deref(storage.const_label);
    } else if (storage.access == ConstStorageAccess::LOAD_INTO_REG) {
        mcode::Operand dst = mcode::Operand::from_register(storage.reg, 4);
        mcode::Operand src = mcode::Operand::from_symbol_deref(storage.const_label);
        lowerer.emit(mcode::Instruction(X8664Opcode::MOVSS, {dst, src}));
        return dst;
    } else if (storage.access == ConstStorageAccess::READ_REG) {
        return mcode::Operand::from_register(storage.reg, 4);
    } else {
        return {};
    }
}

mcode::Value X8664ConstLowering::load_f64(double value) {
    std::string float_label = "float." + std::to_string(cur_id++);
    std::string float_imm = "__float64__(" + std::to_string(value) + ")";
    mcode::Value float_val = mcode::Value::from_immediate(float_imm, 8);
    lowerer.get_machine_module().add(mcode::Global(float_label, float_val));

    return mcode::Operand::from_symbol_deref(float_label);
}

void X8664ConstLowering::process_block() {
    std::map<float, mcode::Register> cur_f32s_in_regs;

    for (ir::InstrIter iter = lowerer.get_block().begin(); iter != lowerer.get_block().end(); ++iter) {
        if (is_discarding_instr(iter->get_opcode())) {
            cur_f32s_in_regs.clear();
        }

        passes::PassUtils::iter_imms(iter->get_operands(), [this, iter, &cur_f32s_in_regs](ir::Value &imm) {
            if (!imm.get_type().is_primitive(ir::Primitive::F32) || !imm.is_fp_immediate()) {
                return;
            }

            float val = (float)imm.get_fp_immediate();
            if (val == 0.0f) {
                return;
            }

            std::string float_label;
            auto const_f32_iter = const_f32s.find(val);

            if (const_f32_iter != const_f32s.end()) {
                float_label = const_f32_iter->second;
            } else {
                float_label = "float." + std::to_string(cur_id++);
                std::string float_imm = "__float32__(" + std::to_string(val) + ")";
                mcode::Value float_val = mcode::Value::from_immediate(float_imm, 4);
                lowerer.get_machine_module().add(mcode::Global(float_label, float_val));
                const_f32s.insert({val, float_label});
            }

            ConstStorage storage;
            auto f32_reg_iter = cur_f32s_in_regs.find(val);

            if (f32_reg_iter != cur_f32s_in_regs.end()) {
                storage = {.access = ConstStorageAccess::READ_REG, .reg = f32_reg_iter->second};
            } else if (is_f32_used_later_on(val, iter) && cur_f32s_in_regs.size() < 4) {
                mcode::Register reg = mcode::Register::from_virtual(lowerer.get_func().next_virtual_reg());
                cur_f32s_in_regs.insert({val, reg});
                storage = {.access = ConstStorageAccess::LOAD_INTO_REG, .const_label = float_label, .reg = reg};
            } else {
                storage = {.access = ConstStorageAccess::LOAD, .const_label = float_label};
            }

            f32_storage[iter][val] = storage;
        });
    }
}

bool X8664ConstLowering::is_f32_used_later_on(float value, ir::InstrIter user) {
    for (ir::InstrIter iter = user; iter != lowerer.get_block().end(); ++iter) {
        if (is_discarding_instr(iter->get_opcode())) {
            return false;
        }

        bool used = false;

        passes::PassUtils::iter_imms(iter->get_operands(), [&used, value](ir::Value &imm) {
            if (imm.get_type().is_primitive(ir::Primitive::F32) && imm.is_fp_immediate() &&
                (float)imm.get_fp_immediate() == value) {
                used = true;
            }
        });

        if (used) {
            return true;
        }
    }

    return false;
}

bool X8664ConstLowering::is_discarding_instr(ir::Opcode opcode) {
    return opcode == ir::Opcode::CALL || opcode == ir::Opcode::COPY;
}

} // namespace target

} // namespace banjo
