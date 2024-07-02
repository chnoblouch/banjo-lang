#include "closure_ir_builder.hpp"

#include "ast/expr.hpp"
#include "ir/structure.hpp"
#include "ir_builder/func_def_ir_builder.hpp"
#include "ir_builder/ir_builder_utils.hpp"
#include "ir_builder/name_mangling.hpp"

#include <utility>

namespace banjo {

namespace ir_builder {

StoredValue ClosureIRBuilder::build(StorageHints hints) {
    lang::ClosureExpr *closure_expr = node->as<lang::ClosureExpr>();

    lang::DataType *lang_type = closure_expr->get_data_type();
    ir::Type type = context.build_type(lang_type);

    StoredValue stored_val = StoredValue::alloc(type, hints, context);
    ir::Value val_ptr = stored_val.get_ptr();
    const ir::Type &val_type = stored_val.value_type;

    lang::Function *enclosing_func = context.get_current_lang_func();
    std::string enclosing_func_name = NameMangling::mangle_func_name(enclosing_func);
    std::string name = enclosing_func_name + ".$" + std::to_string(context.closure_id++);
    closure_expr->get_func()->set_link_name(name);

    std::string struct_name = "closure.struct." + name;
    ir::Structure *struct_ = new ir::Structure(std::move(name));
    context.get_current_mod()->add(struct_);
    ir::Type struct_type(struct_);

    closure = Closure{
        .node = node,
        .context_type = struct_type,
        .captured_vars = {},
    };

    ir::BasicBlockIter enclosing_block = context.get_cur_block_iter();
    std::vector<ir::VirtualRegister> enclosing_arg_regs = context.get_current_arg_regs();
    ir::BasicBlockIter enclosing_func_exit = context.get_cur_func_exit();

    context.set_current_closure(&closure);
    FuncDefIRBuilder(context, node).build_closure(closure_expr->get_func());
    context.set_current_closure(nullptr);

    context.set_current_lang_func(enclosing_func);
    context.set_current_func(enclosing_func->get_ir_func());
    context.set_cur_block_iter(enclosing_block);
    context.set_current_arg_regs(enclosing_arg_regs);
    context.set_cur_func_exit(enclosing_func_exit);

    for (lang::Variable *var : closure.captured_vars) {
        ir::Type type = context.build_type(var->get_data_type());
        struct_->add(ir::StructureMember{var->get_name(), type});
    }

    target::TargetDataLayout &data_layout = context.get_target()->get_data_layout();
    unsigned size = data_layout.get_size(struct_type);

    ir::VirtualRegister data_ptr_reg = context.get_current_func()->next_virtual_reg();
    ir::Value data_ptr = ir::Value::from_register(data_ptr_reg, ir::Primitive::ADDR);
    context.get_cur_block().append(ir::Instruction(
        ir::Opcode::CALL,
        data_ptr_reg,
        {ir::Operand::from_extern_func("malloc", ir::Primitive::ADDR),
         ir::Operand::from_int_immediate(size, data_layout.get_usize_type())}
    ));

    int index = 0;
    for (lang::Variable *var : closure.captured_vars) {
        StoredValue src = var->as_ir_value(context);
        ir::VirtualRegister dst_reg = context.append_memberptr(struct_type, data_ptr, index);
        ir::Operand dst = ir::Operand::from_register(dst_reg, ir::Primitive::ADDR);
        IRBuilderUtils::copy_val(context, src.value_or_ptr, dst, src.value_type);
        index += 1;
    }

    ir::VirtualRegister func_ptr_dest_reg = context.append_memberptr(val_type, val_ptr, 0);
    context.append_store(
        ir::Operand::from_func(closure_expr->get_func()->get_ir_func(), ir::Primitive::ADDR),
        ir::Operand::from_register(func_ptr_dest_reg, ir::Primitive::ADDR)
    );

    ir::VirtualRegister context_ptr_dest_reg = context.append_memberptr(val_type, val_ptr, 1);
    context.append_store(data_ptr, ir::Operand::from_register(context_ptr_dest_reg, ir::Primitive::ADDR));

    return stored_val;
}

} // namespace ir_builder

} // namespace banjo
