#include "closure_ir_builder.hpp"

#include "ast/ast_child_indices.hpp"
#include "ast/expr.hpp"
#include "ir/structure.hpp"
#include "ir_builder/func_def_ir_builder.hpp"
#include "ir_builder/ir_builder_utils.hpp"
#include "ir_builder/name_mangling.hpp"

#include <utility>

namespace ir_builder {

StoredValue ClosureIRBuilder::build(StorageReqs reqs) {
    lang::ClosureExpr *closure_expr = node->as<lang::ClosureExpr>();

    lang::DataType *lang_type = closure_expr->get_data_type();
    ir::Type type = IRBuilderUtils::build_type(lang_type, context, false);

    StoredValue stored_val = StoredValue::alloc(type, reqs, context);
    ir::Value val_ptr = stored_val.value_or_ptr;

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

    context.set_current_closure(&closure);
    FuncDefIRBuilder(context, node).build_closure(closure_expr->get_func());
    context.set_current_closure(nullptr);

    context.set_current_lang_func(enclosing_func);
    context.set_current_func(enclosing_func->get_ir_func());
    context.set_cur_block_iter(enclosing_block);
    context.set_current_arg_regs(enclosing_arg_regs);

    for (lang::Variable *var : closure.captured_vars) {
        ir::Type type = IRBuilderUtils::build_type(var->get_data_type(), context);
        struct_->add(ir::StructureMember{var->get_name(), type});
    }

    target::TargetDataLayout &data_layout = context.get_target()->get_data_layout();
    int size = data_layout.get_size(struct_type, *context.get_current_mod());

    ir::VirtualRegister data_ptr_reg = context.get_current_func()->next_virtual_reg();
    ir::Value data_ptr = ir::Value::from_register(data_ptr_reg, struct_type.ref());
    context.get_cur_block().append(ir::Instruction(
        ir::Opcode::CALL,
        data_ptr_reg,
        {ir::Operand::from_extern_func("malloc", struct_type.ref()),
         ir::Operand::from_int_immediate(size, data_layout.get_usize_type())}
    ));

    int index = 0;
    for (lang::Variable *var : closure.captured_vars) {
        ir::Operand src = var->as_ir_operand(context);
        ir::VirtualRegister dest_reg = context.append_memberptr(data_ptr, index);
        ir::Operand dest = ir::Operand::from_register(dest_reg, src.get_type().ref());
        IRBuilderUtils::copy_val(context, src, dest);
        index += 1;
    }

    ir::VirtualRegister func_ptr_dest_reg = context.append_memberptr(val_ptr, 0);
    context.append_store(
        ir::Operand::from_func(closure_expr->get_func()->get_ir_func(), ir::Type(ir::Primitive::VOID, 1)),
        ir::Operand::from_register(func_ptr_dest_reg, ir::Type(ir::Primitive::VOID, 2))
    );

    ir::VirtualRegister context_ptr_dest_reg = context.append_memberptr(val_ptr, 1);
    context.append_store(data_ptr, ir::Operand::from_register(context_ptr_dest_reg, struct_type.ref()));

    return stored_val;
}

} // namespace ir_builder
