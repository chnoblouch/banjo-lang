#include "constant.hpp"

#include "ir_builder/expr_ir_builder.hpp"

namespace lang {

Constant::Constant() : Variable(nullptr, ""), module_(nullptr) {}

Constant::Constant(ASTNode *node, std::string name, ASTModule *module_) : Variable(node, name), module_(module_) {}

ir::Operand Constant::as_ir_operand(ir_builder::IRBuilderContext &context) {
    return ir_builder::ExprIRBuilder(context, value).build_into_value_if_possible();
}

} // namespace lang
