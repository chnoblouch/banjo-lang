#include "local_variable.hpp"

#include "ir_builder/ir_builder_context.hpp"
#include "ir_builder/ir_builder_utils.hpp"

#include <utility>

namespace lang {

LocalVariable::LocalVariable(ASTNode *node, DataType *data_type, std::string name)
  : Variable(node, data_type, std::move(name)) {}

ir::Operand LocalVariable::as_ir_operand(ir_builder::IRBuilderContext &context) {
    ir::Type type = ir_builder::IRBuilderUtils::build_type(get_data_type()).ref();
    return ir::Operand::from_register(ir::VirtualRegister(virtual_reg_id), type);
}

} // namespace lang
