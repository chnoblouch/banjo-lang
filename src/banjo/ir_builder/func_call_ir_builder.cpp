#include "func_call_ir_builder.hpp"

#include "ast/ast_child_indices.hpp"
#include "ast/expr.hpp"
#include "ir_builder/expr_ir_builder.hpp"
#include "ir_builder/ir_builder_utils.hpp"
#include "ir_builder/location_ir_builder.hpp"
#include "ir_builder/storage.hpp"

namespace banjo {

namespace ir_builder {

ir::VirtualRegister FuncCallIRBuilder::build(StorageHints hints, bool use_result) {
    lang::Expr *location_node = node->get_child(lang::CALL_LOCATION)->as<lang::Expr>();
    lang::ASTNode *args_node = node->get_child(lang::CALL_ARGS);

    LocationIRBuilder location_builder(context, location_node, infer_params());
    StoredValue location = location_builder.build(false);
    lang::Function *func = location_builder.get_lang_func();
    lang::DataType *location_type = location_node->get_location()->get_type();

    type = location_type->get_function_type();
    ir::Type return_type = context.build_type(type.return_type);
    bool return_by_ref = context.get_target()->get_data_layout().is_return_by_ref(return_type);
    bool is_method = func ? func->is_method() : location_node->get_location()->get_last_element().is_proto_method();

    std::vector<ir::Operand> call_instr_operands;

    if (func) {
        ir::Type func_return_type = return_by_ref ? ir::Type(ir::Primitive::VOID) : return_type;

        if (func->get_ir_func()) {
            call_instr_operands.push_back(ir::Operand::from_func(func->get_ir_func(), func_return_type));
        } else {
            std::string name = IRBuilderUtils::get_func_link_name(func);
            call_instr_operands.push_back(ir::Operand::from_extern_func(name, func_return_type));
        }
    } else if (location_type->get_kind() == lang::DataType::Kind::FUNCTION) {
        ir::Value func_ptr = context.append_load(ir::Primitive::ADDR, location.value_or_ptr);
        call_instr_operands.push_back(func_ptr);
    } else if (location_type->get_kind() == lang::DataType::Kind::CLOSURE) {
        ir::Type closure_type = location.value_type;
        ir::VirtualRegister func_ptr_ptr_reg = context.append_memberptr(closure_type, location.value_or_ptr, 0);
        ir::Operand func_ptr_ptr = ir::Operand::from_register(func_ptr_ptr_reg, ir::Primitive::ADDR);
        ir::Value func_ptr = context.append_load(ir::Primitive::ADDR, func_ptr_ptr);

        call_instr_operands.push_back(func_ptr);
    }

    // HACK: don't modify parameters!
    if (!return_type.is_primitive(ir::Primitive::VOID) && !return_by_ref && use_result) {
        hints.dst = ir::Value::from_register(context.get_current_func()->next_virtual_reg());
    }

    ir::VirtualRegister dst_reg;
    if (return_by_ref) {
        StoredValue dst = StoredValue::alloc(return_type, hints, context);
        call_instr_operands.push_back(dst.value_or_ptr);
        dst_reg = dst.value_or_ptr.get_register();
    } else {
        dst_reg = context.get_current_func()->next_virtual_reg();
    }

    if (is_method) {
        call_instr_operands.push_back(get_self_operand(location_builder));
    } else if (location_type->get_kind() == lang::DataType::Kind::CLOSURE) {
        ir::Type closure_type = location.value_type;
        ir::VirtualRegister context_ptr_ptr_reg = context.append_memberptr(closure_type, location.value_or_ptr, 1);
        ir::Operand context_ptr_ptr = ir::Operand::from_register(context_ptr_ptr_reg, ir::Primitive::ADDR);
        ir::Operand context_ptr = context.append_load(ir::Primitive::ADDR, context_ptr_ptr);

        call_instr_operands.push_back(context_ptr);
    }

    for (int i = 0; i < args_node->get_children().size(); i++) {
        lang::ASTNode *argument = args_node->get_child(i);
        call_instr_operands.push_back(ExprIRBuilder(context, argument).build_into_value_if_possible().value_or_ptr);
    }

    if (return_by_ref || !hints.dst) {
        context.get_cur_block().append(ir::Instruction(ir::Opcode::CALL, call_instr_operands));
    } else {
        context.get_cur_block().append(ir::Instruction(ir::Opcode::CALL, dst_reg, call_instr_operands));
    }

    return dst_reg;
}

std::vector<lang::DataType *> FuncCallIRBuilder::infer_params() {
    lang::ASTNode *args_node = node->get_child(lang::CALL_ARGS);
    std::vector<lang::DataType *> params;

    for (lang::ASTNode *arg : args_node->get_children()) {
        lang::DataType *type = arg->as<lang::Expr>()->get_data_type();
        params.push_back(type);
    }

    return params;
}

ir::Operand FuncCallIRBuilder::get_self_operand(LocationIRBuilder &location_builder) {
    const StoredValue &self = location_builder.get_self();

    if (location_builder.get_self().reference) {
        return context.append_load(self.value_type, self.get_ptr());
    } else {
        return location_builder.get_self().get_value();
    }
}

} // namespace ir_builder

} // namespace banjo
