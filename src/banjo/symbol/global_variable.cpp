#include "global_variable.hpp"

#include "ir_builder/ir_builder_context.hpp"
#include "ir_builder/ir_builder_utils.hpp"

#include <utility>

namespace lang {

GlobalVariable::GlobalVariable(ASTNode *node, std::string name, ASTModule *module_)
  : Variable(node, std::move(name)),
    module_(module_) {}

ir::Operand GlobalVariable::as_ir_operand(ir_builder::IRBuilderContext &context) {
    std::string link_name = ir_builder::IRBuilderUtils::get_global_var_link_name(this);
    ir::Type type = ir_builder::IRBuilderUtils::build_type(get_data_type()).ref();
    return native ? ir::Operand::from_extern_global(link_name, type) : ir::Operand::from_global(link_name, type);
}

} // namespace lang
