#include "return_ir_builder.hpp"

#include "banjo/ir_builder/expr_ir_builder.hpp"
#include "banjo/ir_builder/ir_builder_utils.hpp"

namespace banjo {

namespace ir_builder {

void ReturnIRBuilder::build() {
    if (!node->get_children().empty()) {
        build_return_value();
    }

    context.append_jmp(context.get_cur_func_exit());
}

void ReturnIRBuilder::build_return_value() {
    ir::VirtualRegister return_reg = IRBuilderUtils::get_cur_return_reg(context);
    ExprIRBuilder(context, node->get_child()).build_and_store(return_reg);
}

} // namespace ir_builder

} // namespace banjo
