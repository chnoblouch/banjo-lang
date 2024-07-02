#include "constant.hpp"

#include "banjo/ir_builder/expr_ir_builder.hpp"
#include <utility>

namespace banjo {

namespace lang {

Constant::Constant() : Variable(nullptr, ""), module_(nullptr) {}

Constant::Constant(ASTNode *node, std::string name, ASTModule *module_)
  : Variable(node, std::move(name)),
    module_(module_) {}

ir_builder::StoredValue Constant::as_ir_value(ir_builder::IRBuilderContext &context) {
    return ir_builder::ExprIRBuilder(context, value).build();
}

} // namespace lang

} // namespace banjo
