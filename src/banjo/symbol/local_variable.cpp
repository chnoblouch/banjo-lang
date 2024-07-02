#include "local_variable.hpp"

#include "data_type.hpp"
#include "ir_builder/ir_builder_context.hpp"
#include "ir_builder/ir_builder_utils.hpp"
#include "ir_builder/storage.hpp"

#include <utility>

namespace banjo {

namespace lang {

LocalVariable::LocalVariable(ASTNode *node, DataType *data_type, std::string name)
  : Variable(node, data_type, std::move(name)) {}

ir_builder::StoredValue LocalVariable::as_ir_value(ir_builder::IRBuilderContext &context) {
    ir::Type type = context.build_type(get_data_type());
    return ir_builder::StoredValue::create_reference(virtual_reg, type);
}

} // namespace lang

} // namespace banjo
