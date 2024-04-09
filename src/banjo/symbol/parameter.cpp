#include "parameter.hpp"

#include "ir_builder/ir_builder_context.hpp"
#include "ir_builder/ir_builder_utils.hpp"

#include <utility>

namespace lang {

Parameter::Parameter(ASTNode *node, DataType *data_type, std::string name)
  : Variable(node, data_type, std::move(name)) {}

ir::Operand Parameter::as_ir_operand(ir_builder::IRBuilderContext &context) {
    ir::Type type = ir_builder::IRBuilderUtils::build_type(get_data_type(), context).ref();

    if (!pass_by_ref) {
        return ir::Operand::from_register(ir::VirtualRegister(virtual_reg_id), type);
    } else {
        ir::VirtualRegister reg = context.get_current_func()->next_virtual_reg();
        context.append_load(reg, ir::Operand::from_register(ir::VirtualRegister(virtual_reg_id), type.ref()));
        return ir::Operand::from_register(reg, type);
    }
}

} // namespace lang
