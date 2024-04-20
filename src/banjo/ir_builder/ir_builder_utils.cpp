#include "ir_builder_utils.hpp"

#include "ast/expr.hpp"
#include "ir/virtual_register.hpp"
#include "ir_builder/expr_ir_builder.hpp"
#include "ir_builder/ir_builder_context.hpp"
#include "ir_builder/name_mangling.hpp"
#include "symbol/data_type.hpp"
#include "symbol/union.hpp"

namespace ir_builder {

ir::Type IRBuilderUtils::build_type(lang::DataType *type, bool array_to_ptr /* = true */) {
    assert(type && "null type in type ir builder");

    if (type->get_kind() == lang::DataType::Kind::PRIMITIVE) {
        switch (type->get_primitive_type()) {
            case lang::I8: return ir::Type(ir::Primitive::I8);
            case lang::I16: return ir::Type(ir::Primitive::I16);
            case lang::I32: return ir::Type(ir::Primitive::I32);
            case lang::I64: return ir::Type(ir::Primitive::I64);
            case lang::U8: return ir::Type(ir::Primitive::I8);
            case lang::U16: return ir::Type(ir::Primitive::I16);
            case lang::U32: return ir::Type(ir::Primitive::I32);
            case lang::U64: return ir::Type(ir::Primitive::I64);
            case lang::F32: return ir::Type(ir::Primitive::F32);
            case lang::F64: return ir::Type(ir::Primitive::F64);
            case lang::BOOL: return ir::Type(ir::Primitive::I8);
            case lang::ADDR: return ir::Type(ir::Primitive::I64);
            default: return ir::Type(ir::Primitive::VOID);
        }
    } else if (type->get_kind() == lang::DataType::Kind::STRUCT) {
        return ir::Type(type->get_structure()->get_ir_struct());
    } else if (type->get_kind() == lang::DataType::Kind::ENUM) {
        // TODO: what if the value doesn't fit into 32 bits?
        return ir::Type(ir::Primitive::I32);
    } else if (type->get_kind() == lang::DataType::Kind::UNION) {
        return ir::Type(type->get_union()->get_ir_struct());
    } else if (type->get_kind() == lang::DataType::Kind::UNION_CASE) {
        return ir::Type(type->get_union_case()->get_ir_struct());
    } else if (type->get_kind() == lang::DataType::Kind::POINTER) {
        int depth = 0;
        lang::DataType *current_type = type;
        while (current_type->get_kind() == lang::DataType::Kind::POINTER) {
            depth++;
            current_type = current_type->get_base_data_type();
        }

        ir::Type ir_type = build_type(current_type);
        ir_type.set_ptr_depth(depth);
        return ir_type;
    } else if (type->get_kind() == lang::DataType::Kind::FUNCTION) {
        return ir::Type(ir::Primitive::VOID, 1);
    } else if (type->get_kind() == lang::DataType::Kind::STATIC_ARRAY) {
        ir::Type ir_type = build_type(type->get_static_array_type().base_type);

        if (array_to_ptr) {
            return ir_type.ref();
        } else {
            ir_type.set_array_length(type->get_static_array_type().length);
            return ir_type;
        }
    } else if (type->get_kind() == lang::DataType::Kind::TUPLE) {
        std::vector<ir::Type> tuple_types;
        for (lang::DataType *lang_type : type->get_tuple().types) {
            tuple_types.push_back(build_type(lang_type));
        }
        return ir::Type(tuple_types);
    } else if (type->get_kind() == lang::DataType::Kind::CLOSURE) {
        return ir::Type({ir::Type(ir::Primitive::VOID, 1), ir::Type(ir::Primitive::VOID, 1)});
    } else {
        return ir::Type(ir::Primitive::VOID);
    }
}

ir::Type IRBuilderUtils::ptr_to(lang::Variable *var) {
    return build_type(var->get_data_type()).ref();
}

std::vector<ir::Type> IRBuilderUtils::build_params(
    const std::vector<lang::DataType *> &lang_params,
    IRBuilderContext &context
) {
    std::vector<ir::Type> params;
    for (lang::DataType *lang_type : lang_params) {
        ir::Type type = build_type(lang_type);
        if (context.get_target()->get_data_layout().is_pass_by_ref(type)) {
            type = type.ref();
        }

        params.push_back(type);
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

void IRBuilderUtils::store_val(IRBuilderContext &context, ir::Value val, ir::Value dest) {
    target::TargetDataLayout &data_layout = context.get_target()->get_data_layout();

    if (data_layout.fits_in_register(val.get_type())) {
        context.append_store(val, dest);
    } else {
        val.set_type(val.get_type().ref());
        int size = data_layout.get_size(val.get_type().deref());
        ir::Operand symbol_operand = ir::Operand::from_extern_func("memcpy");
        ir::Operand size_operand = ir::Operand::from_int_immediate(size, ir::Type(ir::Primitive::I64));
        context.get_cur_block().append(ir::Instruction(ir::Opcode::CALL, {symbol_operand, dest, val, size_operand}));
    }
}

void IRBuilderUtils::copy_val(IRBuilderContext &context, ir::Value src_ptr, ir::Value dest_ptr) {
    target::TargetDataLayout &data_layout = context.get_target()->get_data_layout();
    ir::Type type(src_ptr.get_type().deref());

    if (data_layout.fits_in_register(type)) {
        ir::VirtualRegister val_reg = context.append_load(src_ptr);
        context.append_store(ir::Operand::from_register(val_reg, type), dest_ptr);
    } else {
        int size = data_layout.get_size(type);
        ir::Operand symbol_operand = ir::Operand::from_extern_func("memcpy");
        ir::Operand size_operand = ir::Operand::from_int_immediate(size, ir::Type(ir::Primitive::I64));
        context.get_cur_block().append(
            ir::Instruction(ir::Opcode::CALL, {symbol_operand, dest_ptr, src_ptr, size_operand})
        );
    }
}

ir::Value IRBuilderUtils::build_arg(lang::ASTNode *node, IRBuilderContext &context) {
    lang::DataType *lang_type = node->as<lang::Expr>()->get_data_type();
    ir::Type type = build_type(lang_type);

    return ExprIRBuilder(context, node).build_into_value_if_possible();
}

StoredValue IRBuilderUtils::build_call(
    lang::Function *func,
    const std::vector<ir::Value> &args,
    IRBuilderContext &context
) {
    return build_call({func, args}, StorageReqs(), context);
}

StoredValue IRBuilderUtils::build_call(FuncCall call, const StorageReqs &reqs, IRBuilderContext &context) {
    ir::Value dst;

    if (reqs.dst) {
        dst = *reqs.dst;
    } else {
        ir::VirtualRegister dst_reg = 0;
        if (!call.func->get_type().return_type->is_primitive_of_type(lang::PrimitiveType::VOID)) {
            dst_reg = context.get_current_func()->next_virtual_reg();
        }

        if (call.func->is_return_by_ref()) {
            ir::Type return_type = IRBuilderUtils::build_type(call.func->get_type().return_type);
            context.append_alloca(dst_reg, return_type);
        }

        dst = ir::Value::from_register(dst_reg, ir::Primitive::VOID);
    }

    return build_call(call, dst, context);
}

StoredValue IRBuilderUtils::build_call(FuncCall call, ir::Value dst, IRBuilderContext &context) {
    std::vector<ir::Value> operands;

    ir::Type built_return_type = IRBuilderUtils::build_type(call.func->get_type().return_type);
    ir::Type return_type = call.func->is_return_by_ref() ? ir::Type(ir::Primitive::VOID) : built_return_type;

    if (call.func->get_ir_func()) {
        operands.push_back(ir::Operand::from_func(call.func->get_ir_func(), return_type));
    } else {
        std::string name = IRBuilderUtils::get_func_link_name(call.func);
        operands.push_back(ir::Operand::from_extern_func(name, return_type));
    }

    if (call.func->is_return_by_ref()) {
        dst.set_type(built_return_type.ref());
        operands.push_back(dst);
    }

    operands.insert(operands.end(), call.args.begin(), call.args.end());

    if (call.func->is_return_by_ref()) {
        context.get_cur_block().append(ir::Instruction(ir::Opcode::CALL, operands));
        dst.set_type(built_return_type.ref());
        return StoredValue::create_ptr(dst, context);
    } else {
        if (call.func->get_type().return_type->is_primitive_of_type(lang::PrimitiveType::VOID)) {
            context.get_cur_block().append(ir::Instruction(ir::Opcode::CALL, operands));
        } else {
            context.get_cur_block().append(ir::Instruction(ir::Opcode::CALL, dst.get_register(), operands));
        }

        dst.set_type(built_return_type);
        return StoredValue::create_value(dst, context);
    }
}

ir::VirtualRegister IRBuilderUtils::get_cur_return_reg(IRBuilderContext &context) {
    if (!context.get_current_lang_func()->is_return_by_ref()) {
        return context.get_current_return_reg();
    } else {
        ir::Type val_ptr_type = context.get_current_func()->get_params()[0].ref();
        ir::Value val_ptr_ptr = ir::Operand::from_register(context.get_current_arg_regs()[0], val_ptr_type.ref());
        return context.append_load(val_ptr_ptr);
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
