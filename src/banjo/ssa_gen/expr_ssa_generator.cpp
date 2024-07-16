#include "expr_ssa_generator.hpp"

#include "banjo/ir/comparison.hpp"
#include "banjo/ir/primitive.hpp"
#include "banjo/ir/virtual_register.hpp"
#include "banjo/sir/sir.hpp"
#include "banjo/ssa_gen/ssa_generator_context.hpp"
#include "banjo/ssa_gen/storage_hints.hpp"
#include "banjo/ssa_gen/stored_value.hpp"
#include "banjo/ssa_gen/type_ssa_generator.hpp"
#include "banjo/utils/macros.hpp"

#include <cassert>
#include <vector>

namespace banjo {

namespace lang {

ExprSSAGenerator::ExprSSAGenerator(SSAGeneratorContext &ctx) : ctx(ctx) {}

StoredValue ExprSSAGenerator::generate(const sir::Expr &expr) {
    return generate(expr, StorageHints::none());
}

StoredValue ExprSSAGenerator::generate_into_value(const sir::Expr &expr) {
    return generate(expr, StorageHints::none()).turn_into_value(ctx);
}

void ExprSSAGenerator::generate_into_dst(const sir::Expr &expr, const ssa::Value &dst) {
    generate(expr, StorageHints::into(dst)).copy_to(dst, ctx);
}

void ExprSSAGenerator::generate_into_dst(const sir::Expr &expr, ssa::VirtualRegister dst) {
    generate_into_dst(expr, ssa::Value::from_register(dst, ssa::Primitive::ADDR));
}

StoredValue ExprSSAGenerator::generate(const sir::Expr &expr, const StorageHints &hints) {
    if (auto int_literal = expr.match<sir::IntLiteral>()) return generate_int_literal(*int_literal);
    else if (auto char_literal = expr.match<sir::CharLiteral>()) return generate_char_literal(*char_literal);
    else if (auto string_literal = expr.match<sir::StringLiteral>()) return generate_string_literal(*string_literal);
    else if (auto struct_literal = expr.match<sir::StructLiteral>())
        return generate_struct_literal(*struct_literal, hints);
    else if (auto ident_expr = expr.match<sir::IdentExpr>()) return generate_ident_expr(*ident_expr);
    else if (auto binary_expr = expr.match<sir::BinaryExpr>()) return generate_binary_expr(*binary_expr);
    else if (auto unary_expr = expr.match<sir::UnaryExpr>()) return generate_unary_expr(*unary_expr);
    else if (auto call_expr = expr.match<sir::CallExpr>()) return generate_call_expr(*call_expr, hints);
    else if (auto dot_expr = expr.match<sir::DotExpr>()) return generate_dot_expr(*dot_expr);
    else ASSERT_UNREACHABLE;
}

void ExprSSAGenerator::generate_bool_expr(const sir::Expr &expr, CondBranchTargets branch_targets) {
    if (auto binary_expr = expr.match<sir::BinaryExpr>()) return generate_comparison(*binary_expr, branch_targets);
    else ASSERT_UNREACHABLE;
}

StoredValue ExprSSAGenerator::generate_int_literal(const sir::IntLiteral &int_literal) {
    ssa::Type ssa_type = TypeSSAGenerator(ctx).generate(int_literal.type);
    ssa::Value ssa_immediate = ssa::Value::from_int_immediate(int_literal.value, ssa_type);
    return StoredValue::create_value(ssa_immediate);
}

StoredValue ExprSSAGenerator::generate_char_literal(const sir::CharLiteral &char_literal) {
    unsigned value = static_cast<unsigned>(char_literal.value);
    ssa::Type ssa_type = TypeSSAGenerator(ctx).generate(char_literal.type);
    ssa::Value ssa_immediate = ssa::Value::from_int_immediate(value, ssa_type);
    return StoredValue::create_value(ssa_immediate);
}

StoredValue ExprSSAGenerator::generate_string_literal(const sir::StringLiteral &string_literal) {
    std::string ssa_name = ctx.next_string_name();
    ssa::Type ssa_type = ssa::Primitive::ADDR;
    ssa::Value ssa_value = ssa::Operand::from_string(string_literal.value + std::string(1, '\0'));
    ctx.ssa_mod->add(ssa::Global(ssa_name, ssa_type, ssa_value));

    ssa::Value ssa_global_val = ssa::Operand::from_global(ssa_name, ssa_type);
    return StoredValue::create_value(ssa_global_val);
}

StoredValue ExprSSAGenerator::generate_struct_literal(
    const sir::StructLiteral &struct_literal,
    const StorageHints &hints
) {
    ssa::Type ssa_type = TypeSSAGenerator(ctx).generate(struct_literal.type);
    StoredValue stored_val = StoredValue::alloc(ssa_type, hints, ctx);

    for (const sir::StructLiteralEntry &entry : struct_literal.entries) {
        unsigned index = entry.field->index;
        ssa::VirtualRegister ssa_field_ptr = ctx.append_memberptr(stored_val.value_type, stored_val.get_ptr(), index);
        ssa::Type ssa_field_type = TypeSSAGenerator(ctx).generate(entry.field->type);
        ExprSSAGenerator(ctx).generate_into_dst(entry.value, ssa::Value::from_register(ssa_field_ptr, ssa_field_type));
    }

    return stored_val;
}

StoredValue ExprSSAGenerator::generate_ident_expr(const sir::IdentExpr &ident_expr) {
    if (auto func_def = ident_expr.symbol.match<sir::FuncDef>()) {
        ssa::Function *ssa_func = ctx.ssa_funcs[func_def];
        ssa::Value ssa_value = ssa::Value::from_func(ssa_func, ssa::Primitive::ADDR);
        return StoredValue::create_value(ssa_value);
    } else if (auto native_func_decl = ident_expr.symbol.match<sir::NativeFuncDecl>()) {
        ssa::Value ssa_value = ssa::Value::from_extern_func(native_func_decl->ident.value, ssa::Primitive::ADDR);
        return StoredValue::create_value(ssa_value);
    } else if (auto var_stmt = ident_expr.symbol.match<sir::VarStmt>()) {
        ssa::VirtualRegister reg = ctx.ssa_var_regs[var_stmt];
        ssa::Type ssa_type = TypeSSAGenerator(ctx).generate(var_stmt->type);
        ssa::Value ssa_ptr = ssa::Value::from_register(reg, ssa::Primitive::ADDR);
        return StoredValue::create_reference(ssa_ptr, ssa_type);
    } else if (auto param = ident_expr.symbol.match<sir::Param>()) {
        ssa::VirtualRegister slot = ctx.ssa_param_slots[param];
        ssa::Value ssa_ptr = ssa::Value::from_register(slot, ssa::Primitive::ADDR);
        ssa::Type ssa_type = TypeSSAGenerator(ctx).generate(param->type);
        return StoredValue::create_reference(ssa_ptr, ssa_type);
    } else {
        ASSERT_UNREACHABLE;
    }
}

StoredValue ExprSSAGenerator::generate_binary_expr(const sir::BinaryExpr &binary_expr) {
    ssa::Opcode ssa_op;
    switch (binary_expr.op) {
        case sir::BinaryOp::ADD: ssa_op = ssa::Opcode::ADD; break;
        case sir::BinaryOp::SUB: ssa_op = ssa::Opcode::SUB; break;
        case sir::BinaryOp::MUL: ssa_op = ssa::Opcode::MUL; break;
        case sir::BinaryOp::DIV: ssa_op = ssa::Opcode::SDIV; break;
        case sir::BinaryOp::MOD: ssa_op = ssa::Opcode::SREM; break;
        case sir::BinaryOp::BIT_AND: ssa_op = ssa::Opcode::AND; break;
        case sir::BinaryOp::BIT_OR: ssa_op = ssa::Opcode::OR; break;
        case sir::BinaryOp::BIT_XOR: ssa_op = ssa::Opcode::XOR; break;
        case sir::BinaryOp::SHL: ssa_op = ssa::Opcode::SHL; break;
        case sir::BinaryOp::SHR: ssa_op = ssa::Opcode::SHR; break;
        default: return {};
    }

    ssa::VirtualRegister reg = ctx.next_vreg();
    ssa::Value ssa_lhs = generate(binary_expr.lhs).turn_into_value(ctx).get_value();
    ssa::Value ssa_rhs = generate(binary_expr.rhs).turn_into_value(ctx).get_value();

    ctx.get_ssa_block()->append({ssa_op, reg, {ssa_lhs, ssa_rhs}});

    ssa::Type ssa_type = TypeSSAGenerator(ctx).generate(binary_expr.type);
    return StoredValue::create_value(reg, ssa_type);
}

StoredValue ExprSSAGenerator::generate_unary_expr(const sir::UnaryExpr &unary_expr) {
    switch (unary_expr.op) {
        case sir::UnaryOp::NEG: ASSERT_UNREACHABLE;
        case sir::UnaryOp::REF: return generate_ref(unary_expr);
        case sir::UnaryOp::DEREF: return generate_deref(unary_expr);
        case sir::UnaryOp::NOT: ASSERT_UNREACHABLE;
    }
}

StoredValue ExprSSAGenerator::generate_ref(const sir::UnaryExpr &unary_expr) {
    StoredValue ssa_val = generate(unary_expr.value, StorageHints::prefer_reference());
    return StoredValue::create_value(ssa_val.get_ptr());
}

StoredValue ExprSSAGenerator::generate_deref(const sir::UnaryExpr &unary_expr) {
    StoredValue ssa_val = generate(unary_expr.value).try_turn_into_value(ctx);
    ssa::Type ssa_type = TypeSSAGenerator(ctx).generate(unary_expr.value.get_type());
    return StoredValue::create_reference(ssa_val.value_or_ptr, ssa_type);
}

StoredValue ExprSSAGenerator::generate_call_expr(const sir::CallExpr &call_expr, const StorageHints &hints) {
    ssa::Value ssa_callee = generate(call_expr.callee).turn_into_value(ctx).get_value();
    ssa::Type ssa_type = TypeSSAGenerator(ctx).generate(call_expr.type);
    ReturnMethod return_method = ctx.get_return_method(ssa_type);

    std::vector<ssa::Value> ssa_operands;
    StoredValue return_value_ptr;

    if (return_method == ReturnMethod::VIA_POINTER_ARG) {
        ssa_operands.push_back(ssa_callee.with_type(ssa::Primitive::ADDR));
        return_value_ptr = StoredValue::alloc(ssa_type, hints, ctx);
        ssa_operands.push_back(return_value_ptr.get_ptr());
    } else {
        ssa_operands.push_back(ssa_callee.with_type(ssa_type));
    }

    for (const sir::Expr &arg : call_expr.args) {
        ssa_operands.push_back(generate(arg).turn_into_value(ctx).get_value());
    }

    if (return_method == ReturnMethod::NO_RETURN_VALUE || hints.is_unused) {
        assert(!hints.dst);
        ctx.get_ssa_block()->append({ssa::Opcode::CALL, ssa_operands});
        return StoredValue::create_value({});
    } else if (return_method == ReturnMethod::IN_REGISTER) {
        ssa::VirtualRegister dst = hints.dst ? hints.dst->get_register() : ctx.next_vreg();
        ctx.get_ssa_block()->append({ssa::Opcode::CALL, dst, ssa_operands});
        return StoredValue::create_value(dst, ssa_type);
    } else if (return_method == ReturnMethod::VIA_POINTER_ARG) {
        ctx.get_ssa_block()->append({ssa::Opcode::CALL, ssa_operands});
        return return_value_ptr;
    } else {
        ASSERT_UNREACHABLE;
    }
}

StoredValue ExprSSAGenerator::generate_dot_expr(const sir::DotExpr &dot_expr) {
    const sir::StructField &field = dot_expr.symbol.as<sir::StructField>();

    StoredValue ssa_lhs = ExprSSAGenerator(ctx).generate(dot_expr.lhs, StorageHints::prefer_reference());
    ssa::VirtualRegister ssa_field_ptr = ctx.append_memberptr(ssa_lhs.value_type, ssa_lhs.get_ptr(), field.index);
    ssa::Type ssa_type = TypeSSAGenerator(ctx).generate(field.type);

    return StoredValue::create_reference(ssa_field_ptr, ssa_type);
}

void ExprSSAGenerator::generate_comparison(const sir::BinaryExpr &binary_expr, CondBranchTargets branch_targets) {
    ssa::Comparison ssa_cmp;
    switch (binary_expr.op) {
        case sir::BinaryOp::EQ: ssa_cmp = ssa::Comparison::EQ; break;
        case sir::BinaryOp::NE: ssa_cmp = ssa::Comparison::NE; break;
        case sir::BinaryOp::GT: ssa_cmp = ssa::Comparison::SGT; break;
        case sir::BinaryOp::LT: ssa_cmp = ssa::Comparison::SLT; break;
        case sir::BinaryOp::GE: ssa_cmp = ssa::Comparison::SGE; break;
        case sir::BinaryOp::LE: ssa_cmp = ssa::Comparison::SLE; break;
        default: return;
    }

    ssa::Value ssa_lhs = generate(binary_expr.lhs).turn_into_value(ctx).get_value();
    ssa::Value ssa_rhs = generate(binary_expr.rhs).turn_into_value(ctx).get_value();

    ctx.append_cjmp(ssa_lhs, ssa_cmp, ssa_rhs, branch_targets.target_if_true, branch_targets.target_if_false);
}

} // namespace lang

} // namespace banjo
