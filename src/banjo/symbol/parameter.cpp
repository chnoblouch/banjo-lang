#include "parameter.hpp"

#include "ir_builder/ir_builder_context.hpp"
#include "ir_builder/ir_builder_utils.hpp"
#include "ir_builder/storage.hpp"

#include <utility>

namespace banjo {

namespace lang {

Parameter::Parameter(ASTNode *node, DataType *data_type, std::string name)
  : Variable(node, data_type, std::move(name)) {}

ir_builder::StoredValue Parameter::as_ir_value(ir_builder::IRBuilderContext &context) {
    ir::Value value;

    if (!pass_by_ref) {
        value = ir::Operand::from_register(virtual_reg, ir::Primitive::ADDR);
    } else {
        ir::Operand ptr = ir::Operand::from_register(virtual_reg, ir::Primitive::ADDR);
        value = context.append_load(ir::Primitive::ADDR, ptr);
    }

    ir::Type type = context.build_type(get_data_type());
    return ir_builder::StoredValue::create_reference(value, type);
}

} // namespace lang

} // namespace banjo
