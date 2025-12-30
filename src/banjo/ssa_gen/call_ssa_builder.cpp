#include "call_ssa_builder.hpp"

namespace banjo::lang {

CallSSABuilder::CallSSABuilder(SSAGeneratorContext &ctx, ssa::Type ssa_return_type, const StorageHints &hints)
  : ctx{ctx},
    ssa_return_type{ssa_return_type},
    hints{hints} {}

CallSSABuilder &CallSSABuilder::set_callee(const ssa::Value &ssa_callee) {
    return_method = ctx.get_return_method(ssa_return_type);

    if (return_method == ReturnMethod::VIA_POINTER_ARG) {
        ssa_operands.push_back(ssa_callee.with_type(ssa::Primitive::VOID));
        return_value_ptr = StoredValue::alloc(ssa_return_type, hints, ctx);
        ssa_operands.push_back(return_value_ptr.get_ptr());
    } else {
        ssa_operands.push_back(ssa_callee.with_type(ssa_return_type));
    }

    return *this;
}

CallSSABuilder &CallSSABuilder::add_arg(StoredValue value) {
    ssa::Type ssa_type = value.value_type;
    target::ArgPassMethod pass_method = ctx.target->get_data_layout().get_arg_pass_method(ssa_type);

    if (pass_method.num_args == 1) {
        ssa_operands.push_back(value.turn_into_value_or_copy(ctx).value_or_ptr);
    } else {
        StoredValue ssa_arg = value.turn_into_reference(ctx);
        ASSERT(ssa_arg.kind == StoredValue::Kind::REFERENCE);

        for (unsigned i = 0; i < pass_method.num_args; i++) {
            ssa::VirtualRegister ptr = ctx.append_offsetptr(ssa_arg.get_ptr(), i, ssa::Primitive::U64);
            ssa::Type type = i == pass_method.num_args - 1 ? pass_method.last_arg_type : ssa::Primitive::U64;
            ssa::Value value = ctx.append_load(type, ptr);
            ssa_operands.push_back(value);
        }
    }

    return *this;
}

StoredValue CallSSABuilder::generate() {
    if (return_method == ReturnMethod::NO_RETURN_VALUE || hints.is_unused) {
        assert(!hints.dst);
        ctx.get_ssa_block()->append({ssa::Opcode::CALL, std::move(ssa_operands)});
        return StoredValue::create_value({});
    } else if (return_method == ReturnMethod::IN_REGISTER) {
        ssa::VirtualRegister dst = ctx.next_vreg();
        ctx.get_ssa_block()->append({ssa::Opcode::CALL, dst, std::move(ssa_operands)});
        return StoredValue::create_value(dst, ssa_return_type);
    } else if (return_method == ReturnMethod::VIA_POINTER_ARG) {
        ctx.get_ssa_block()->append({ssa::Opcode::CALL, std::move(ssa_operands)});
        return return_value_ptr;
    } else {
        ASSERT_UNREACHABLE;
    }
}

} // namespace banjo::lang
