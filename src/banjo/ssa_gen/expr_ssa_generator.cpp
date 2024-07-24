#include "expr_ssa_generator.hpp"

#include "banjo/ir/basic_block.hpp"
#include "banjo/ir/comparison.hpp"
#include "banjo/ir/instruction.hpp"
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
    else if (auto fp_literal = expr.match<sir::FPLiteral>()) return generate_fp_literal(*fp_literal);
    else if (auto bool_literal = expr.match<sir::BoolLiteral>()) return generate_bool_literal(*bool_literal);
    else if (auto char_literal = expr.match<sir::CharLiteral>()) return generate_char_literal(*char_literal);
    else if (auto string_literal = expr.match<sir::StringLiteral>()) return generate_string_literal(*string_literal);
    else if (auto struct_literal = expr.match<sir::StructLiteral>())
        return generate_struct_literal(*struct_literal, hints);
    else if (auto symbol_expr = expr.match<sir::SymbolExpr>()) return generate_symbol_expr(*symbol_expr);
    else if (auto binary_expr = expr.match<sir::BinaryExpr>()) return generate_binary_expr(*binary_expr, expr);
    else if (auto unary_expr = expr.match<sir::UnaryExpr>()) return generate_unary_expr(*unary_expr, expr);
    else if (auto cast_expr = expr.match<sir::CastExpr>()) return generate_cast_expr(*cast_expr);
    else if (auto index_expr = expr.match<sir::IndexExpr>()) return generate_index_expr(*index_expr);
    else if (auto call_expr = expr.match<sir::CallExpr>()) return generate_call_expr(*call_expr, hints);
    else if (auto field_expr = expr.match<sir::FieldExpr>()) return generate_field_expr(*field_expr);
    else ASSERT_UNREACHABLE;
}

void ExprSSAGenerator::generate_branch(const sir::Expr &expr, CondBranchTargets branch_targets) {
    if (auto binary_expr = expr.match<sir::BinaryExpr>()) {
        switch (binary_expr->op) {
            case sir::BinaryOp::EQ: return generate_cmp_branch(*binary_expr, ssa::Comparison::EQ, branch_targets);
            case sir::BinaryOp::NE: return generate_cmp_branch(*binary_expr, ssa::Comparison::NE, branch_targets);
            case sir::BinaryOp::GT: return generate_cmp_branch(*binary_expr, ssa::Comparison::SGT, branch_targets);
            case sir::BinaryOp::LT: return generate_cmp_branch(*binary_expr, ssa::Comparison::SLT, branch_targets);
            case sir::BinaryOp::GE: return generate_cmp_branch(*binary_expr, ssa::Comparison::SGE, branch_targets);
            case sir::BinaryOp::LE: return generate_cmp_branch(*binary_expr, ssa::Comparison::SLE, branch_targets);
            case sir::BinaryOp::AND: return generate_and_branch(*binary_expr, branch_targets);
            case sir::BinaryOp::OR: return generate_or_branch(*binary_expr, branch_targets);
            default: break;
        }
    } else if (auto unary_expr = expr.match<sir::UnaryExpr>()) {
        if (unary_expr->op == sir::UnaryOp::NOT) {
            return generate_not_branch(*unary_expr, branch_targets);
        }
    }

    return generate_zero_check_branch(expr, branch_targets);
}

StoredValue ExprSSAGenerator::generate_int_literal(const sir::IntLiteral &int_literal) {
    ssa::Type ssa_type = TypeSSAGenerator(ctx).generate(int_literal.type);
    ssa::Value ssa_immediate = ssa::Value::from_int_immediate(int_literal.value, ssa_type);
    return StoredValue::create_value(ssa_immediate);
}

StoredValue ExprSSAGenerator::generate_fp_literal(const sir::FPLiteral &fp_literal) {
    ssa::Type ssa_type = TypeSSAGenerator(ctx).generate(fp_literal.type);
    ssa::Value ssa_immediate = ssa::Value::from_fp_immediate(fp_literal.value, ssa_type);
    return StoredValue::create_value(ssa_immediate);
}

StoredValue ExprSSAGenerator::generate_bool_literal(const sir::BoolLiteral &bool_literal) {
    unsigned value = bool_literal.value ? 1 : 0;
    ssa::Type ssa_type = TypeSSAGenerator(ctx).generate(bool_literal.type);
    ssa::Value ssa_immediate = ssa::Value::from_int_immediate(value, ssa_type);
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

StoredValue ExprSSAGenerator::generate_symbol_expr(const sir::SymbolExpr &symbol_expr) {
    if (auto func_def = symbol_expr.symbol.match<sir::FuncDef>()) {
        ssa::Function *ssa_func = ctx.ssa_funcs[func_def];
        ssa::Value ssa_value = ssa::Value::from_func(ssa_func, ssa::Primitive::ADDR);
        return StoredValue::create_value(ssa_value);
    } else if (auto native_func_decl = symbol_expr.symbol.match<sir::NativeFuncDecl>()) {
        ssa::Value ssa_value = ssa::Value::from_extern_func(native_func_decl->ident.value, ssa::Primitive::ADDR);
        return StoredValue::create_value(ssa_value);
    } else if (auto var_stmt = symbol_expr.symbol.match<sir::VarStmt>()) {
        ssa::VirtualRegister reg = ctx.ssa_var_regs[var_stmt];
        ssa::Type ssa_type = TypeSSAGenerator(ctx).generate(var_stmt->type);
        ssa::Value ssa_ptr = ssa::Value::from_register(reg, ssa::Primitive::ADDR);
        return StoredValue::create_reference(ssa_ptr, ssa_type);
    } else if (auto param = symbol_expr.symbol.match<sir::Param>()) {
        ssa::VirtualRegister slot = ctx.ssa_param_slots[param];
        ssa::Value ssa_ptr = ssa::Value::from_register(slot, ssa::Primitive::ADDR);
        ssa::Type ssa_type = TypeSSAGenerator(ctx).generate(param->type);
        return StoredValue::create_reference(ssa_ptr, ssa_type);
    } else {
        ASSERT_UNREACHABLE;
    }
}

StoredValue ExprSSAGenerator::generate_binary_expr(const sir::BinaryExpr &binary_expr, const sir::Expr &expr) {
    switch (binary_expr.op) {
        case sir::BinaryOp::EQ:
        case sir::BinaryOp::NE:
        case sir::BinaryOp::GT:
        case sir::BinaryOp::LT:
        case sir::BinaryOp::GE:
        case sir::BinaryOp::LE: return generate_bool_expr(expr);
        default: break;
    }

    ssa::Opcode ssa_op;

    if (binary_expr.lhs.get_type().is_int_type()) {
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
            default: ASSERT_UNREACHABLE;
        }
    } else if (binary_expr.lhs.get_type().is_fp_type()) {
        switch (binary_expr.op) {
            case sir::BinaryOp::ADD: ssa_op = ssa::Opcode::FADD; break;
            case sir::BinaryOp::SUB: ssa_op = ssa::Opcode::FSUB; break;
            case sir::BinaryOp::MUL: ssa_op = ssa::Opcode::FMUL; break;
            case sir::BinaryOp::DIV: ssa_op = ssa::Opcode::FDIV; break;
            default: ASSERT_UNREACHABLE;
        }
    } else {
        ASSERT_UNREACHABLE;
    }

    ssa::VirtualRegister reg = ctx.next_vreg();
    ssa::Value ssa_lhs = generate(binary_expr.lhs).turn_into_value(ctx).get_value();
    ssa::Value ssa_rhs = generate(binary_expr.rhs).turn_into_value(ctx).get_value();

    ctx.get_ssa_block()->append({ssa_op, reg, {ssa_lhs, ssa_rhs}});

    ssa::Type ssa_type = TypeSSAGenerator(ctx).generate(binary_expr.type);
    return StoredValue::create_value(reg, ssa_type);
}

StoredValue ExprSSAGenerator::generate_unary_expr(const sir::UnaryExpr &unary_expr, const sir::Expr &expr) {
    switch (unary_expr.op) {
        case sir::UnaryOp::NEG: return generate_neg(unary_expr);
        case sir::UnaryOp::REF: return generate_ref(unary_expr);
        case sir::UnaryOp::DEREF: return generate_deref(unary_expr);
        case sir::UnaryOp::NOT: return generate_bool_expr(expr);
    }
}

StoredValue ExprSSAGenerator::generate_neg(const sir::UnaryExpr &unary_expr) {
    ssa::Value ssa_val = generate(unary_expr.value).turn_into_value(ctx).get_value();
    ssa::VirtualRegister ssa_out_reg = ctx.next_vreg();

    if (unary_expr.value.get_type().is_int_type()) {
        ssa::Value ssa_zero = ssa::Value::from_int_immediate(0, ssa_val.get_type());
        ctx.get_ssa_block()->append(ssa::Instruction(ssa::Opcode::SUB, ssa_out_reg, {ssa_zero, ssa_val}));
    } else if (unary_expr.value.get_type().is_fp_type()) {
        ssa::Value ssa_zero = ssa::Value::from_fp_immediate(0.0, ssa_val.get_type());
        ctx.get_ssa_block()->append(ssa::Instruction(ssa::Opcode::FSUB, ssa_out_reg, {ssa_zero, ssa_val}));
    } else {
        ASSERT_UNREACHABLE;
    }

    ssa::Value ssa_out_val = ssa::Value::from_register(ssa_out_reg, ssa_val.get_type());
    return StoredValue::create_value(ssa_out_val);
}

StoredValue ExprSSAGenerator::generate_ref(const sir::UnaryExpr &unary_expr) {
    StoredValue ssa_val = generate(unary_expr.value, StorageHints::prefer_reference());
    return StoredValue::create_value(ssa_val.get_ptr());
}

StoredValue ExprSSAGenerator::generate_deref(const sir::UnaryExpr &unary_expr) {
    StoredValue ssa_val = generate(unary_expr.value).try_turn_into_value(ctx);
    ssa::Type ssa_type = TypeSSAGenerator(ctx).generate(unary_expr.type);
    return StoredValue::create_reference(ssa_val.value_or_ptr, ssa_type);
}

StoredValue ExprSSAGenerator::generate_cast_expr(const sir::CastExpr &cast_expr) {
    const sir::Expr &sir_type_from = cast_expr.value.get_type();
    const sir::Expr &sir_type_to = cast_expr.type;

    ssa::Value ssa_val_in = generate(cast_expr.value).turn_into_value(ctx).get_value();
    ssa::Type ssa_type_to = TypeSSAGenerator(ctx).generate(sir_type_to);

    unsigned size_from = ctx.target->get_data_layout().get_size(ssa_val_in.get_type());
    unsigned size_to = ctx.target->get_data_layout().get_size(ssa_type_to);

    if (size_from == size_to && sir_type_from.is_fp_type() == sir_type_to.is_fp_type()) {
        return StoredValue::create_value(ssa_val_in);
    }

    ssa::Opcode ssa_op;

    if (sir_type_from.is_fp_type()) {
        if (sir_type_to.is_fp_type()) {
            ssa_op = size_to > size_from ? ir::Opcode::FPROMOTE : ir::Opcode::FDEMOTE;
        } else {
            ssa_op = sir_type_to.is_signed_type() ? ir::Opcode::FTOS : ir::Opcode::FTOU;
        }
    } else {
        if (sir_type_to.is_fp_type()) {
            ssa_op = sir_type_from.is_signed_type() ? ir::Opcode::STOF : ir::Opcode::UTOF;
        } else if (size_to > size_from) {
            ssa_op = sir_type_from.is_signed_type() ? ir::Opcode::SEXTEND : ir::Opcode::UEXTEND;
        } else {
            ssa_op = ir::Opcode::TRUNCATE;
        }
    }

    ssa::VirtualRegister ssa_reg_out = ctx.next_vreg();
    ssa::Value ssa_type_val = ssa::Value::from_type(ssa_type_to);
    ctx.get_ssa_block()->append(ssa::Instruction(ssa_op, ssa_reg_out, {ssa_val_in, ssa_type_val}));

    ssa::Value ssa_val_out = ssa::Value::from_register(ssa_reg_out, ssa_type_to);
    return StoredValue::create_value(ssa_val_out);
}

StoredValue ExprSSAGenerator::generate_index_expr(const sir::IndexExpr &index_expr) {
    ssa::Value ssa_base = generate(index_expr.base).turn_into_value(ctx).get_value();
    ssa::Type ssa_type = TypeSSAGenerator(ctx).generate(index_expr.type);
    ssa::Value ssa_offset = generate(index_expr.index).turn_into_value(ctx).get_value();
    ssa::VirtualRegister ssa_reg = ctx.append_offsetptr(ssa_base, ssa_offset, ssa_type);
    return StoredValue::create_reference(ssa::Value::from_register(ssa_reg, ssa::Primitive::ADDR), ssa_type);
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
        ssa::VirtualRegister dst = ctx.next_vreg();
        ctx.get_ssa_block()->append({ssa::Opcode::CALL, dst, ssa_operands});
        return StoredValue::create_value(dst, ssa_type);
    } else if (return_method == ReturnMethod::VIA_POINTER_ARG) {
        ctx.get_ssa_block()->append({ssa::Opcode::CALL, ssa_operands});
        return return_value_ptr;
    } else {
        ASSERT_UNREACHABLE;
    }
}

StoredValue ExprSSAGenerator::generate_field_expr(const sir::FieldExpr &field_expr) {
    const sir::StructField &field = *field_expr.field;

    StoredValue ssa_lhs = ExprSSAGenerator(ctx).generate(field_expr.base, StorageHints::prefer_reference());
    ssa::VirtualRegister ssa_field_ptr = ctx.append_memberptr(ssa_lhs.value_type, ssa_lhs.get_ptr(), field.index);
    ssa::Type ssa_type = TypeSSAGenerator(ctx).generate(field.type);

    return StoredValue::create_reference(ssa_field_ptr, ssa_type);
}

StoredValue ExprSSAGenerator::generate_bool_expr(const sir::Expr &expr) {
    ssa::BasicBlockIter ssa_target_if_true = ctx.create_block();
    ssa::BasicBlockIter ssa_target_if_false = ctx.create_block();
    ssa::BasicBlockIter ssa_end = ctx.create_block();

    generate_branch(expr, {ssa_target_if_true, ssa_target_if_false});

    ssa::VirtualRegister ssa_slot_reg = ctx.append_alloca(ssa::Primitive::I8);
    ssa::Value ssa_slot = ssa::Value::from_register(ssa_slot_reg, ssa::Primitive::ADDR);

    ctx.append_block(ssa_target_if_true);
    ctx.append_store(ssa::Operand::from_int_immediate(1, ssa::Primitive::I8), ssa_slot);
    ctx.append_jmp(ssa_end);
    ctx.append_block(ssa_target_if_false);
    ctx.append_store(ssa::Operand::from_int_immediate(0, ssa::Primitive::I8), ssa_slot);
    ctx.append_jmp(ssa_end);
    ctx.append_block(ssa_end);

    return StoredValue::create_reference(ssa_slot, ssa::Primitive::I8);
}

void ExprSSAGenerator::generate_zero_check_branch(const sir::Expr &expr, CondBranchTargets branch_targets) {
    ssa::Value ssa_value = generate(expr).turn_into_value(ctx).get_value();
    ssa::Comparison ssa_cmp = ssa::Comparison::NE;
    ssa::Value ssa_zero = ssa::Value::from_int_immediate(0, ssa_value.get_type());
    ctx.append_cjmp(ssa_value, ssa_cmp, ssa_zero, branch_targets.target_if_true, branch_targets.target_if_false);
}

void ExprSSAGenerator::generate_cmp_branch(
    const sir::BinaryExpr &binary_expr,
    ssa::Comparison ssa_cmp,
    CondBranchTargets branch_targets
) {
    ssa::Value ssa_lhs = generate(binary_expr.lhs).turn_into_value(ctx).get_value();
    ssa::Value ssa_rhs = generate(binary_expr.rhs).turn_into_value(ctx).get_value();
    ctx.append_cjmp(ssa_lhs, ssa_cmp, ssa_rhs, branch_targets.target_if_true, branch_targets.target_if_false);
}

void ExprSSAGenerator::generate_and_branch(const sir::BinaryExpr &binary_expr, CondBranchTargets branch_targets) {
    ssa::BasicBlockIter ssa_rhs_block = ctx.create_block();
    generate_branch(binary_expr.lhs, {ssa_rhs_block, branch_targets.target_if_false});
    ctx.append_block(ssa_rhs_block);
    generate_branch(binary_expr.rhs, {branch_targets.target_if_true, branch_targets.target_if_false});
}

void ExprSSAGenerator::generate_or_branch(const sir::BinaryExpr &binary_expr, CondBranchTargets branch_targets) {
    ssa::BasicBlockIter ssa_rhs_block = ctx.create_block();
    generate_branch(binary_expr.lhs, {branch_targets.target_if_true, ssa_rhs_block});
    ctx.append_block(ssa_rhs_block);
    generate_branch(binary_expr.rhs, {branch_targets.target_if_true, branch_targets.target_if_false});
}

void ExprSSAGenerator::generate_not_branch(const sir::UnaryExpr &unary_expr, CondBranchTargets branch_targets) {
    generate_branch(unary_expr.value, {branch_targets.target_if_false, branch_targets.target_if_true});
}

} // namespace lang

} // namespace banjo
