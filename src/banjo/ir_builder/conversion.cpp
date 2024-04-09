#include "conversion.hpp"

#include "ir_builder/ir_builder_utils.hpp"

namespace ir_builder {

ir::Value Conversion::build(IRBuilderContext &context, ir::Value &value, lang::DataType *from, lang::DataType *to) {
    ir::Type ir_from = IRBuilderUtils::build_type(from, context);
    ir::Type ir_to = IRBuilderUtils::build_type(to, context);

    target::TargetDataLayout &data_layout = context.get_target()->get_data_layout();
    int size_from = data_layout.get_size(ir_from, *context.get_current_mod());
    int size_to = data_layout.get_size(ir_to, *context.get_current_mod());

    bool is_from_signed = from->is_signed_int();
    bool is_to_signed = to->is_signed_int();
    bool is_promotion = size_to > size_from;

    if (size_from == size_to && from->is_floating_point() == to->is_floating_point()) {
        ir::Value copy = value;
        copy.set_type(ir_to);
        return copy;
    } else if (value.is_int_immediate() && to->is_floating_point()) {
        double converted_value = std::stod(value.get_int_immediate().to_string());
        return ir::Value::from_fp_immediate(converted_value, ir_to);
    }

    ir::Opcode opcode;

    if (from->is_floating_point()) {
        if (to->is_floating_point()) {
            opcode = is_promotion ? ir::Opcode::FPROMOTE : ir::Opcode::FDEMOTE;
        } else {
            opcode = is_to_signed ? ir::Opcode::FTOS : ir::Opcode::FTOU;
        }
    } else {
        if (to->is_floating_point()) {
            opcode = is_from_signed ? ir::Opcode::STOF : ir::Opcode::UTOF;
        } else if (is_promotion) {
            opcode = is_from_signed ? ir::Opcode::SEXTEND : ir::Opcode::UEXTEND;
        } else {
            opcode = ir::Opcode::TRUNCATE;
        }
    }

    ir::VirtualRegister reg = context.get_current_func()->next_virtual_reg();
    context.get_cur_block().append(ir::Instruction(opcode, reg, {value, ir::Operand::from_type(ir_to)}));
    return ir::Value::from_register(reg, ir_to);
}

} // namespace ir_builder
