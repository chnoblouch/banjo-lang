#include "parameter.hpp"

#include "ir_builder/ir_builder_context.hpp"
#include "ir_builder/ir_builder_utils.hpp"
#include "ir_builder/storage.hpp"

#include <utility>

namespace lang {

Parameter::Parameter(ASTNode *node, DataType *data_type, std::string name)
  : Variable(node, data_type, std::move(name)) {}

ir_builder::StoredValue Parameter::as_ir_value(ir_builder::IRBuilderContext &context) {
    ir::Type type = ir_builder::IRBuilderUtils::build_type(get_data_type());
    ir::Value value;

    if (!pass_by_ref) {
        value = ir::Operand::from_register(virtual_reg, type.ref());
    } else {
        ir::VirtualRegister reg = context.get_current_func()->next_virtual_reg();
        context.append_load(reg, ir::Operand::from_register(virtual_reg, type.ref().ref()));
        value = ir::Operand::from_register(reg, type.ref());
    }

    return ir_builder::StoredValue::create_reference(value, type);
}

} // namespace lang
