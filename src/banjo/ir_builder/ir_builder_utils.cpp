#include "ir_builder_utils.hpp"

#include "banjo/ast/expr.hpp"
#include "banjo/ir/primitive.hpp"
#include "banjo/ir/virtual_register.hpp"
#include "banjo/ir_builder/expr_ir_builder.hpp"
#include "banjo/ir_builder/ir_builder_context.hpp"
#include "banjo/ir_builder/name_mangling.hpp"
#include "banjo/symbol/data_type.hpp"
#include "banjo/symbol/union.hpp"

#include <cassert>

namespace banjo {

namespace ir_builder {

std::vector<ir::Type> IRBuilderUtils::build_params(
    const std::vector<lang::DataType *> &lang_params,
    IRBuilderContext &context
) {
    std::vector<ir::Type> params;

    for (lang::DataType *lang_type : lang_params) {
        ir::Type type = context.build_type(lang_type);

        if (context.get_target()->get_data_layout().is_pass_by_ref(type)) {
            params.push_back(ir::Primitive::ADDR);
        } else {
            params.push_back(type);
        }
    }

    return params;
}

std::string IRBuilderUtils::get_func_link_name(lang::Function *func) {
    // TODO: name mangling also does native check
    if (func->get_link_name().has_value()) {
        return func->get_link_name().value();
    } else if (func->is_native() || func->is_exposed() || func->is_dllexport()) {
        return func->get_name();
    } else {
        return NameMangling::mangle_func_name(func);
    }
}

std::string IRBuilderUtils::get_global_var_link_name(lang::GlobalVariable *global_var) {
    // TODO: name mangling also does native check
    if (global_var->get_link_name().has_value()) {
        return global_var->get_link_name().value();
    } else if (global_var->is_native() || global_var->is_exposed()) {
        return global_var->get_name();
    } else {
        return NameMangling::mangle_global_var_name(global_var);
    }
}

void IRBuilderUtils::copy_val(IRBuilderContext &context, ir::Value src_ptr, ir::Value dst_ptr, ir::Type type) {
    target::TargetDataLayout &data_layout = context.get_target()->get_data_layout();

    if (data_layout.fits_in_register(type)) {
        ir::Value val = context.append_load(type, std::move(src_ptr));
        context.append_store(val, std::move(dst_ptr));
    } else {
        unsigned size = data_layout.get_size(type);
        ir::Operand symbol_operand = ir::Operand::from_extern_func("memcpy");
        ir::Operand size_operand = ir::Operand::from_int_immediate(size, ir::Primitive::I64);
        context.get_cur_block().append(
            ir::Instruction(ir::Opcode::CALL, {symbol_operand, std::move(dst_ptr), std::move(src_ptr), size_operand})
        );
    }
}

ir::Value IRBuilderUtils::build_arg(lang::ASTNode *node, IRBuilderContext &context) {
    lang::DataType *lang_type = node->as<lang::Expr>()->get_data_type();
    ir::Type type = context.build_type(lang_type);

    return ExprIRBuilder(context, node).build_into_value_if_possible().value_or_ptr;
}

StoredValue IRBuilderUtils::build_call(
    lang::Function *func,
    const std::vector<ir::Value> &args,
    IRBuilderContext &context
) {
    return build_call({func, args}, context);
}

StoredValue IRBuilderUtils::build_call(FuncCall call, IRBuilderContext &context) {
    ir::VirtualRegister dst_reg = 0;
    if (!call.func->get_type().return_type->is_primitive_of_type(lang::PrimitiveType::VOID)) {
        dst_reg = context.get_current_func()->next_virtual_reg();
    }

    if (call.func->is_return_by_ref()) {
        ir::Type return_type = context.build_type(call.func->get_type().return_type);
        context.append_alloca(dst_reg, return_type);
    }

    ir::Value dst = ir::Value::from_register(dst_reg, ir::Primitive::VOID);
    return build_call(call, dst, context);
}

StoredValue IRBuilderUtils::build_call(FuncCall call, ir::Value dst, IRBuilderContext &context) {
    std::vector<ir::Value> operands;

    ir::Type built_return_type = context.build_type(call.func->get_type().return_type);
    ir::Type return_type = call.func->is_return_by_ref() ? ir::Type(ir::Primitive::VOID) : built_return_type;

    if (call.func->get_ir_func()) {
        operands.push_back(ir::Operand::from_func(call.func->get_ir_func(), return_type));
    } else {
        std::string name = IRBuilderUtils::get_func_link_name(call.func);
        operands.push_back(ir::Operand::from_extern_func(name, return_type));
    }

    if (call.func->is_return_by_ref()) {
        dst.set_type(ir::Primitive::ADDR);
        operands.push_back(dst);
    }

    operands.insert(operands.end(), call.args.begin(), call.args.end());

    if (call.func->is_return_by_ref()) {
        context.get_cur_block().append(ir::Instruction(ir::Opcode::CALL, operands));
        dst.set_type(ir::Primitive::ADDR);
        return StoredValue::create_reference(dst, built_return_type);
    } else {
        if (call.func->get_type().return_type->is_primitive_of_type(lang::PrimitiveType::VOID)) {
            context.get_cur_block().append(ir::Instruction(ir::Opcode::CALL, operands));
        } else {
            context.get_cur_block().append(ir::Instruction(ir::Opcode::CALL, dst.get_register(), operands));
        }

        dst.set_type(built_return_type);
        return StoredValue::create_value(dst);
    }
}

ir::VirtualRegister IRBuilderUtils::get_cur_return_reg(IRBuilderContext &context) {
    if (!context.get_current_lang_func()->is_return_by_ref()) {
        return context.get_current_return_reg();
    } else {
        ir::Value ptr = ir::Operand::from_register(context.get_current_arg_regs()[0], ir::Primitive::ADDR);
        return context.append_load(ir::Primitive::ADDR, ptr).get_register();
    }
}

bool IRBuilderUtils::is_branch(ir::Instruction &instr) {
    ir::Opcode opcode = instr.get_opcode();
    return opcode == ir::Opcode::JMP || opcode == ir::Opcode::CJMP || opcode == ir::Opcode::FCJMP;
}

bool IRBuilderUtils::is_branching(ir::BasicBlock &block) {
    return block.get_instrs().get_size() != 0 && is_branch(block.get_instrs().get_last());
}

} // namespace ir_builder

} // namespace banjo
