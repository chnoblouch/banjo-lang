#include "expr_ssa_generator.hpp"

#include "banjo/sir/sir.hpp"
#include "banjo/sir/sir_visitor.hpp"
#include "banjo/ssa/basic_block.hpp"
#include "banjo/ssa/comparison.hpp"
#include "banjo/ssa/instruction.hpp"
#include "banjo/ssa/primitive.hpp"
#include "banjo/ssa/virtual_register.hpp"
#include "banjo/ssa_gen/name_mangling.hpp"
#include "banjo/ssa_gen/ssa_generator_context.hpp"
#include "banjo/ssa_gen/storage_hints.hpp"
#include "banjo/ssa_gen/stored_value.hpp"
#include "banjo/ssa_gen/type_ssa_generator.hpp"
#include "banjo/target/target_data_layout.hpp"
#include "banjo/utils/macros.hpp"

#include <cassert>
#include <vector>

namespace banjo {

namespace lang {

ExprSSAGenerator::ExprSSAGenerator(SSAGeneratorContext &ctx) : ctx(ctx) {}

StoredValue ExprSSAGenerator::generate(const sir::Expr &expr) {
    return generate(expr, StorageHints::none());
}

void ExprSSAGenerator::generate_into_dst(const sir::Expr &expr, const ssa::Value &dst) {
    generate(expr, StorageHints::into(dst)).copy_to(dst, ctx);
}

void ExprSSAGenerator::generate_into_dst(const sir::Expr &expr, ssa::VirtualRegister dst) {
    generate_into_dst(expr, ssa::Value::from_register(dst, ssa::Primitive::ADDR));
}

StoredValue ExprSSAGenerator::generate_as_reference(const sir::Expr &expr) {
    return generate(expr, StorageHints::prefer_reference()).turn_into_reference(ctx);
}

StoredValue ExprSSAGenerator::generate(const sir::Expr &expr, const StorageHints &hints) {
    SIR_VISIT_EXPR(
        expr,
        SIR_VISIT_IMPOSSIBLE,                              // empty
        return generate_int_literal(*inner),               // int_literal
        return generate_fp_literal(*inner),                // fp_literal
        return generate_bool_literal(*inner),              // bool_literal
        return generate_char_literal(*inner),              // char_literal
        return generate_null_literal(*inner),              // null_literal
        SIR_VISIT_IMPOSSIBLE,                              // none_literal
        return generate_undefined_literal(*inner),         // undefined_literal
        return generate_array_literal(*inner, hints),      // array_literal
        return generate_string_literal(*inner),            // string_literal
        return generate_struct_literal(*inner, hints),     // struct_literal
        return generate_union_case_literal(*inner, hints), // union_case_literal
        SIR_VISIT_IMPOSSIBLE,                              // map_literal
        SIR_VISIT_IMPOSSIBLE,                              // closure_literal
        return generate_symbol_expr(*inner),               // symbol_expr
        return generate_binary_expr(*inner, expr),         // binary_expr
        return generate_unary_expr(*inner, expr),          // unary_expr
        return generate_cast_expr(*inner),                 // cast_expr
        return generate_index_expr(*inner),                // index_expr
        return generate_call_expr(*inner, hints),          // call_expr
        return generate_field_expr(*inner),                // field_expr
        SIR_VISIT_IMPOSSIBLE,                              // range_expr
        return generate_tuple_expr(*inner, hints),         // tuple_expr
        return generate_coercion_expr(*inner, hints),      // coercion_expr
        SIR_VISIT_IMPOSSIBLE,                              // primitive_type
        SIR_VISIT_IMPOSSIBLE,                              // pointer_type
        SIR_VISIT_IMPOSSIBLE,                              // static_array_type
        SIR_VISIT_IMPOSSIBLE,                              // func_type
        SIR_VISIT_IMPOSSIBLE,                              // optional_type
        SIR_VISIT_IMPOSSIBLE,                              // result_type
        SIR_VISIT_IMPOSSIBLE,                              // array_type
        SIR_VISIT_IMPOSSIBLE,                              // map_type
        SIR_VISIT_IMPOSSIBLE,                              // closure_type
        SIR_VISIT_IMPOSSIBLE,                              // reference_type
        SIR_VISIT_IMPOSSIBLE,                              // ident_expr
        SIR_VISIT_IMPOSSIBLE,                              // star_expr
        SIR_VISIT_IMPOSSIBLE,                              // bracket_expr
        SIR_VISIT_IMPOSSIBLE,                              // dot_expr
        SIR_VISIT_IMPOSSIBLE,                              // pseudo_type
        SIR_VISIT_IMPOSSIBLE,                              // meta_access
        SIR_VISIT_IMPOSSIBLE,                              // meta_field_expr
        SIR_VISIT_IMPOSSIBLE,                              // meta_call_expr
        return generate_init_expr(*inner, hints),          // init_expr
        return generate_move_expr(*inner, hints),          // move_expr
        return generate_deinit_expr(*inner),               // deinit_expr
        SIR_VISIT_IMPOSSIBLE                               // error
    );
}

void ExprSSAGenerator::generate_branch(const sir::Expr &expr, CondBranchTargets targets) {
    if (auto binary_expr = expr.match<sir::BinaryExpr>()) {
        if (!binary_expr->lhs.get_type().is_fp_type()) {
            switch (binary_expr->op) {
                case sir::BinaryOp::EQ: return generate_int_cmp_branch(*binary_expr, ssa::Comparison::EQ, targets);
                case sir::BinaryOp::NE: return generate_int_cmp_branch(*binary_expr, ssa::Comparison::NE, targets);
                case sir::BinaryOp::GT: return generate_int_cmp_branch(*binary_expr, ssa::Comparison::SGT, targets);
                case sir::BinaryOp::LT: return generate_int_cmp_branch(*binary_expr, ssa::Comparison::SLT, targets);
                case sir::BinaryOp::GE: return generate_int_cmp_branch(*binary_expr, ssa::Comparison::SGE, targets);
                case sir::BinaryOp::LE: return generate_int_cmp_branch(*binary_expr, ssa::Comparison::SLE, targets);
                case sir::BinaryOp::AND: return generate_and_branch(*binary_expr, targets);
                case sir::BinaryOp::OR: return generate_or_branch(*binary_expr, targets);
                default: ASSERT_UNREACHABLE;
            }
        } else {
            switch (binary_expr->op) {
                case sir::BinaryOp::EQ: return generate_fp_cmp_branch(*binary_expr, ssa::Comparison::FEQ, targets);
                case sir::BinaryOp::NE: return generate_fp_cmp_branch(*binary_expr, ssa::Comparison::FNE, targets);
                case sir::BinaryOp::GT: return generate_fp_cmp_branch(*binary_expr, ssa::Comparison::FGT, targets);
                case sir::BinaryOp::LT: return generate_fp_cmp_branch(*binary_expr, ssa::Comparison::FLT, targets);
                case sir::BinaryOp::GE: return generate_fp_cmp_branch(*binary_expr, ssa::Comparison::FGE, targets);
                case sir::BinaryOp::LE: return generate_fp_cmp_branch(*binary_expr, ssa::Comparison::FLE, targets);
                default: ASSERT_UNREACHABLE;
            }
        }
    } else if (auto unary_expr = expr.match<sir::UnaryExpr>()) {
        if (unary_expr->op == sir::UnaryOp::NOT) {
            return generate_not_branch(*unary_expr, targets);
        }
    }

    return generate_zero_check_branch(expr, targets);
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

StoredValue ExprSSAGenerator::generate_null_literal(const sir::NullLiteral &null_literal) {
    ssa::Type ssa_type = TypeSSAGenerator(ctx).generate(null_literal.type);
    ssa::Value ssa_immediate = ssa::Value::from_int_immediate(0, ssa_type);
    return StoredValue::create_value(ssa_immediate);
}

StoredValue ExprSSAGenerator::generate_undefined_literal(const sir::UndefinedLiteral &undefined_literal) {
    ssa::Type ssa_type = TypeSSAGenerator(ctx).generate(undefined_literal.type);
    return StoredValue::create_undefined(ssa_type);
}

StoredValue ExprSSAGenerator::generate_array_literal(
    const sir::ArrayLiteral &array_literal,
    const StorageHints &hints
) {
    const sir::Expr &element_type = array_literal.type.as<sir::StaticArrayType>().base_type;

    ssa::Type ssa_array_type = TypeSSAGenerator(ctx).generate(array_literal.type);
    ssa::Type ssa_element_type = TypeSSAGenerator(ctx).generate(element_type);
    StoredValue stored_val = StoredValue::alloc(ssa_array_type, hints, ctx);

    for (unsigned i = 0; i < array_literal.values.size(); i++) {
        const sir::Expr &value = array_literal.values[i];
        ssa::VirtualRegister ssa_element_ptr_reg = ctx.append_offsetptr(stored_val.get_ptr(), i, ssa_element_type);
        ssa::Value ssa_element_ptr = ssa::Value::from_register(ssa_element_ptr_reg, ssa::Primitive::ADDR);
        ExprSSAGenerator(ctx).generate_into_dst(value, ssa_element_ptr);
    }

    return stored_val;
}

StoredValue ExprSSAGenerator::generate_string_literal(const sir::StringLiteral &string_literal) {
    std::string ssa_name = ctx.next_string_name();
    ssa::Type ssa_type = ssa::Primitive::ADDR;

    ssa::Global *ssa_global = new ssa::Global{
        .name = ssa_name,
        .type = ssa_type,
        .initial_value = std::string{string_literal.value} + '\0',
        .external = false,
    };

    ctx.ssa_mod->add(ssa_global);
    ssa::Value ssa_global_val = ssa::Operand::from_global(ssa_global, ssa_type);
    return StoredValue::create_value(ssa_global_val);
}

StoredValue ExprSSAGenerator::generate_struct_literal(
    const sir::StructLiteral &struct_literal,
    const StorageHints &hints
) {
    const sir::StructDef &sir_struct_def = struct_literal.type.as_symbol<sir::StructDef>();
    sir::Attributes::Layout layout = sir_struct_def.get_layout();

    ssa::Type ssa_type = TypeSSAGenerator(ctx).generate(struct_literal.type);
    StoredValue stored_val = StoredValue::alloc(ssa_type, hints, ctx);

    for (const sir::StructLiteralEntry &entry : struct_literal.entries) {
        ssa::Value ssa_field_ptr = generate_field_access(stored_val, entry.field->index, layout);
        ExprSSAGenerator(ctx).generate_into_dst(entry.value, ssa_field_ptr);
    }

    return stored_val;
}

StoredValue ExprSSAGenerator::generate_union_case_literal(
    const sir::UnionCaseLiteral &union_case_literal,
    const StorageHints &hints
) {
    ssa::Type ssa_type = TypeSSAGenerator(ctx).generate(union_case_literal.type);
    StoredValue ssa_value = StoredValue::alloc(ssa_type, hints, ctx);

    for (unsigned i = 0; i < union_case_literal.args.size(); i++) {
        ssa::VirtualRegister ssa_field_ptr_reg = ctx.append_memberptr(ssa_value.value_type, ssa_value.get_ptr(), i);
        ssa::Value ssa_field_ptr = ssa::Value::from_register(ssa_field_ptr_reg, ssa::Primitive::ADDR);
        ExprSSAGenerator(ctx).generate_into_dst(union_case_literal.args[i], ssa_field_ptr);
    }

    return ssa_value;
}

StoredValue ExprSSAGenerator::generate_symbol_expr(const sir::SymbolExpr &symbol_expr) {
    if (auto func_def = symbol_expr.symbol.match<sir::FuncDef>()) {
        ssa::Function *ssa_func = ctx.ssa_funcs[func_def];
        ssa::Value ssa_value = ssa::Value::from_func(ssa_func, ssa::Primitive::ADDR);
        return StoredValue::create_value(ssa_value);
    } else if (auto native_func_decl = symbol_expr.symbol.match<sir::NativeFuncDecl>()) {
        ssa::FunctionDecl *ssa_func = ctx.ssa_native_funcs[native_func_decl];
        ssa::Value ssa_value = ssa::Value::from_extern_func(ssa_func, ssa::Primitive::ADDR);
        return StoredValue::create_value(ssa_value);
    } else if (auto const_def = symbol_expr.symbol.match<sir::ConstDef>()) {
        return generate(const_def->value);
    } else if (auto var_decl = symbol_expr.symbol.match<sir::VarDecl>()) {
        unsigned index = ctx.ssa_globals[var_decl];
        ssa::Global *ssa_global = ctx.ssa_mod->get_globals()[index];
        ssa::Type ssa_type = TypeSSAGenerator(ctx).generate(var_decl->type);
        ssa::Value ssa_ptr = ssa::Value::from_global(ssa_global, ssa::Primitive::ADDR);
        return StoredValue::create_reference(ssa_ptr, ssa_type);
    } else if (auto native_var_decl = symbol_expr.symbol.match<sir::NativeVarDecl>()) {
        unsigned index = ctx.ssa_extern_globals[native_var_decl];
        ssa::GlobalDecl *ssa_extern_global = ctx.ssa_mod->get_external_globals()[index];
        ssa::Type ssa_type = TypeSSAGenerator(ctx).generate(native_var_decl->type);
        ssa::Value ssa_ptr = ssa::Value::from_extern_global(ssa_extern_global, ssa::Primitive::ADDR);
        return StoredValue::create_reference(ssa_ptr, ssa_type);
    } else if (auto enum_variant = symbol_expr.symbol.match<sir::EnumVariant>()) {
        return generate(enum_variant->value).turn_into_value(ctx);
    } else if (auto local = symbol_expr.symbol.match<sir::Local>()) {
        ssa::VirtualRegister reg = ctx.ssa_local_regs[local];
        ssa::Type ssa_type = TypeSSAGenerator(ctx).generate(local->type);
        ssa::Value ssa_ptr = ssa::Value::from_register(reg, ssa::Primitive::ADDR);
        return StoredValue::create_reference(ssa_ptr, ssa_type);
    } else if (auto param = symbol_expr.symbol.match<sir::Param>()) {
        return generate_param_expr(*param);
    } else {
        ASSERT_UNREACHABLE;
    }
}

StoredValue ExprSSAGenerator::generate_param_expr(const sir::Param &param) {
    ssa::VirtualRegister slot = ctx.ssa_param_slots[&param];
    ssa::Type ssa_type = TypeSSAGenerator(ctx).generate(param.type);
    ssa::Value ssa_ptr = ssa::Value::from_register(slot, ssa::Primitive::ADDR);

    target::ArgPassMethod pass_method = ctx.target->get_data_layout().get_arg_pass_method(ssa_type);
    StoredValue ssa_value;

    if (pass_method.via_pointer) {
        ssa::Value ssa_loaded_ptr = ctx.append_load(ssa::Primitive::ADDR, ssa_ptr);
        ssa_value = StoredValue::create_reference(ssa_loaded_ptr, ssa_type);
    } else {
        ssa_value = StoredValue::create_reference(ssa_ptr, ssa_type);
    }

    return ssa_value;
}

StoredValue ExprSSAGenerator::generate_binary_expr(const sir::BinaryExpr &binary_expr, const sir::Expr &expr) {
    switch (binary_expr.op) {
        case sir::BinaryOp::EQ:
        case sir::BinaryOp::NE:
        case sir::BinaryOp::GT:
        case sir::BinaryOp::LT:
        case sir::BinaryOp::GE:
        case sir::BinaryOp::LE:
        case sir::BinaryOp::AND:
        case sir::BinaryOp::OR: return generate_bool_expr(expr);
        default: break;
    }

    ssa::Value ssa_lhs = generate(binary_expr.lhs).turn_into_value(ctx).get_value();
    ssa::Value ssa_rhs = generate(binary_expr.rhs).turn_into_value(ctx).get_value();
    ssa::VirtualRegister reg;

    if (binary_expr.lhs.get_type().is_int_type()) {
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
            default: ASSERT_UNREACHABLE;
        }

        reg = ctx.next_vreg();
        ctx.get_ssa_block()->append({ssa_op, reg, {ssa_lhs, ssa_rhs}});
    } else if (binary_expr.lhs.get_type().is_fp_type()) {
        ssa::Opcode ssa_op;

        switch (binary_expr.op) {
            case sir::BinaryOp::ADD: ssa_op = ssa::Opcode::FADD; break;
            case sir::BinaryOp::SUB: ssa_op = ssa::Opcode::FSUB; break;
            case sir::BinaryOp::MUL: ssa_op = ssa::Opcode::FMUL; break;
            case sir::BinaryOp::DIV: ssa_op = ssa::Opcode::FDIV; break;
            default: ASSERT_UNREACHABLE;
        }

        reg = ctx.next_vreg();
        ctx.get_ssa_block()->append({ssa_op, reg, {ssa_lhs, ssa_rhs}});
    } else if (auto pointer_type = binary_expr.lhs.get_type().match<sir::PointerType>()) {
        ssa::Type ssa_base_type = TypeSSAGenerator(ctx).generate(pointer_type->base_type);
        reg = generate_pointer_expr(binary_expr.op, ssa_lhs, ssa_rhs, ssa_base_type);
    } else if (binary_expr.lhs.get_type().is_primitive_type(sir::Primitive::ADDR)) {
        reg = generate_pointer_expr(binary_expr.op, ssa_lhs, ssa_rhs, ssa::Primitive::U8);
    } else {
        ASSERT_UNREACHABLE;
    }

    ssa::Type ssa_type = TypeSSAGenerator(ctx).generate(binary_expr.type);
    return StoredValue::create_value(reg, ssa_type);
}

StoredValue ExprSSAGenerator::generate_unary_expr(const sir::UnaryExpr &unary_expr, const sir::Expr &expr) {
    switch (unary_expr.op) {
        case sir::UnaryOp::NEG: return generate_neg(unary_expr);
        case sir::UnaryOp::BIT_NOT: return generate_bit_not(unary_expr);
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

StoredValue ExprSSAGenerator::generate_bit_not(const sir::UnaryExpr &unary_expr) {
    ssa::Value ssa_val = generate(unary_expr.value).turn_into_value(ctx).get_value();
    unsigned size = ctx.target->get_data_layout().get_size(ssa_val.get_type());

    std::uint64_t immediate;

    switch (size) {
        case 0: immediate = 0; break; // TODO
        case 1: immediate = 0xFF; break;
        case 2: immediate = 0xFFFF; break;
        case 4: immediate = 0xFFFFFFFF; break;
        case 8: immediate = 0xFFFFFFFFFFFFFFFF; break;
    }

    ssa::Value ssa_imm = ssa::Value::from_int_immediate(immediate, ssa_val.get_type());

    ssa::VirtualRegister ssa_out_reg = ctx.next_vreg();
    ctx.get_ssa_block()->append(ssa::Instruction(ssa::Opcode::XOR, ssa_out_reg, {ssa_imm, ssa_val}));
    ssa::Value ssa_out_val = ssa::Value::from_register(ssa_out_reg, ssa_val.get_type());

    return StoredValue::create_value(ssa_out_val);
}

StoredValue ExprSSAGenerator::generate_ref(const sir::UnaryExpr &unary_expr) {
    StoredValue ssa_val = generate_as_reference(unary_expr.value);
    return StoredValue::create_value(ssa_val.get_ptr());
}

StoredValue ExprSSAGenerator::generate_deref(const sir::UnaryExpr &unary_expr) {
    StoredValue ssa_val = generate(unary_expr.value).try_turn_into_value(ctx);
    ssa::Type ssa_type = TypeSSAGenerator(ctx).generate(unary_expr.type);
    return StoredValue::create_reference(ssa_val.value_or_ptr, ssa_type);
}

StoredValue ExprSSAGenerator::generate_cast_expr(const sir::CastExpr &cast_expr) {
    const sir::Expr &sir_type_from = cast_expr.value.get_type();
    ssa::Value ssa_val_from = generate(cast_expr.value).turn_into_value(ctx).get_value();
    unsigned size_from = ctx.target->get_data_layout().get_size(ssa_val_from.get_type());
    bool is_from_fp = sir_type_from.is_fp_type();

    const sir::Expr &sir_type_to = cast_expr.type;
    ssa::Type ssa_type_to = TypeSSAGenerator(ctx).generate(sir_type_to);
    unsigned size_to = ctx.target->get_data_layout().get_size(ssa_type_to);
    bool is_to_fp = sir_type_to.is_fp_type();

    if (size_from == size_to && is_from_fp == is_to_fp) {
        return StoredValue::create_value(ssa_val_from);
    }

    if (ssa_val_from.is_int_immediate()) {
        ssa::Value immediate;

        if (!is_to_fp) {
            immediate = ssa_val_from.with_type(ssa_type_to);
        } else if (ssa_val_from.get_int_immediate().is_positive()) {
            immediate = ssa::Value::from_fp_immediate(ssa_val_from.get_int_immediate().get_magnitude(), ssa_type_to);
        } else if (ssa_val_from.get_int_immediate().is_negative()) {
            immediate = ssa::Value::from_fp_immediate(-ssa_val_from.get_int_immediate().get_magnitude(), ssa_type_to);
        }

        return StoredValue::create_value(immediate);
    }

    ssa::Opcode ssa_op;

    if (is_from_fp) {
        if (is_to_fp) {
            ssa_op = size_to > size_from ? ssa::Opcode::FPROMOTE : ssa::Opcode::FDEMOTE;
        } else {
            ssa_op = sir_type_to.is_signed_type() ? ssa::Opcode::FTOS : ssa::Opcode::FTOU;
        }
    } else {
        if (is_to_fp) {
            ssa_op = sir_type_from.is_signed_type() ? ssa::Opcode::STOF : ssa::Opcode::UTOF;
        } else if (size_to > size_from) {
            ssa_op = sir_type_from.is_signed_type() ? ssa::Opcode::SEXTEND : ssa::Opcode::UEXTEND;
        } else {
            ssa_op = ssa::Opcode::TRUNCATE;
        }
    }

    ssa::VirtualRegister ssa_reg_out = ctx.next_vreg();
    ssa::Value ssa_type_val = ssa::Value::from_type(ssa_type_to);
    ctx.get_ssa_block()->append(ssa::Instruction(ssa_op, ssa_reg_out, {ssa_val_from, ssa_type_val}));

    ssa::Value ssa_val_out = ssa::Value::from_register(ssa_reg_out, ssa_type_to);
    return StoredValue::create_value(ssa_val_out);
}

StoredValue ExprSSAGenerator::generate_index_expr(const sir::IndexExpr &index_expr) {
    const sir::Expr &base_type = index_expr.base.get_type();
    ssa::Type ssa_type = TypeSSAGenerator(ctx).generate(index_expr.type);
    ssa::Value ssa_offset = generate(index_expr.index).turn_into_value(ctx).get_value();

    if (base_type.is<sir::PointerType>()) {
        ssa::Value ssa_base = generate(index_expr.base).turn_into_value(ctx).get_value();
        ssa::VirtualRegister ssa_reg = ctx.append_offsetptr(ssa_base, ssa_offset, ssa_type);
        return StoredValue::create_reference(ssa::Value::from_register(ssa_reg, ssa::Primitive::ADDR), ssa_type);
    } else if (base_type.is<sir::StaticArrayType>()) {
        ssa::Value ssa_base = generate(index_expr.base).get_ptr();
        ssa::VirtualRegister ssa_reg = ctx.append_offsetptr(ssa_base, ssa_offset, ssa_type);
        return StoredValue::create_reference(ssa::Value::from_register(ssa_reg, ssa::Primitive::ADDR), ssa_type);
    } else {
        ASSERT_UNREACHABLE;
    }
}

StoredValue ExprSSAGenerator::generate_call_expr(const sir::CallExpr &call_expr, const StorageHints &hints) {
    ssa::Value ssa_callee;
    std::optional<ssa::Value> ssa_proto_self;

    sir::Symbol callee_symbol = nullptr;
    sir::ProtoDef *proto_def = nullptr;

    if (auto symbol_expr = call_expr.callee.match<sir::SymbolExpr>()) {
        callee_symbol = symbol_expr->symbol;

        sir::Symbol callee_symbol_parent = nullptr;
        bool is_method = false;

        if (auto func_def = callee_symbol.match<sir::FuncDef>()) {
            callee_symbol_parent = func_def->parent;
            is_method = func_def->is_method();
        } else if (auto func_decl = callee_symbol.match<sir::FuncDecl>()) {
            callee_symbol_parent = func_decl->parent;
            is_method = func_decl->is_method();
        }

        if (callee_symbol_parent && callee_symbol_parent.is<sir::ProtoDef>() && is_method) {
            proto_def = &callee_symbol_parent.as<sir::ProtoDef>();
        }
    }

    if (proto_def) {
        unsigned index = *proto_def->get_index(callee_symbol.get_ident().value);
        ssa::Type vtable_type = ctx.ssa_vtable_types[proto_def];

        ssa_proto_self = generate(call_expr.args[0]).turn_into_reference(ctx).get_ptr();

        ssa::Value vtable_ptr_ptr = ctx.append_memberptr_val(ctx.get_fat_pointer_type(), *ssa_proto_self, 1);
        ssa::Value vtable_ptr = ctx.append_load(ssa::Primitive::ADDR, vtable_ptr_ptr);
        ssa::Value proto_func_ptr = ctx.append_memberptr_val(vtable_type, vtable_ptr, index);
        ssa_callee = ctx.append_load(ssa::Primitive::ADDR, proto_func_ptr);
    } else {
        ssa_callee = generate(call_expr.callee).turn_into_value(ctx).get_value();
    }

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

    if (!ssa_proto_self) {
        for (const sir::Expr &arg : call_expr.args) {
            ssa::Type ssa_type = TypeSSAGenerator(ctx).generate(arg.get_type());
            target::ArgPassMethod pass_method = ctx.target->get_data_layout().get_arg_pass_method(ssa_type);

            if (pass_method.num_args == 1) {
                ssa_operands.push_back(generate(arg).turn_into_value_or_copy(ctx).value_or_ptr);
            } else {
                StoredValue ssa_arg = generate(arg).turn_into_value_or_copy(ctx);
                ASSERT(ssa_arg.kind == StoredValue::Kind::REFERENCE);

                for (unsigned i = 0; i < pass_method.num_args; i++) {
                    ssa::VirtualRegister ptr = ctx.append_offsetptr(ssa_arg.get_ptr(), i, ssa::Primitive::U64);
                    ssa::Type type = i == pass_method.num_args - 1 ? pass_method.last_arg_type : ssa::Primitive::U64;
                    ssa::Value value = ctx.append_load(type, ptr);
                    ssa_operands.push_back(value);
                }
            }
        }
    } else {
        ssa::Value data_ptr_ptr = ctx.append_memberptr_val(ctx.get_fat_pointer_type(), *ssa_proto_self, 0);
        ssa::Value data_ptr = ctx.append_load(ssa::Primitive::ADDR, data_ptr_ptr);
        ssa_operands.push_back(data_ptr);

        for (unsigned i = 1; i < call_expr.args.size(); i++) {
            ssa_operands.push_back(generate(call_expr.args[i]).turn_into_value_or_copy(ctx).value_or_ptr);
        }
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
    sir::Attributes::Layout base_type_layout = get_type_layout(field_expr.base.get_type());

    StoredValue ssa_lhs = ExprSSAGenerator(ctx).generate_as_reference(field_expr.base);
    ssa::Value ssa_field_ptr = generate_field_access(ssa_lhs, field_expr.field_index, base_type_layout);
    ssa::Type ssa_type = TypeSSAGenerator(ctx).generate(field_expr.type);

    return StoredValue::create_reference(ssa_field_ptr, ssa_type);
}

StoredValue ExprSSAGenerator::generate_tuple_expr(const sir::TupleExpr &tuple_expr, const StorageHints &hints) {
    ssa::Type ssa_type = TypeSSAGenerator(ctx).generate(tuple_expr.type);
    StoredValue stored_val = StoredValue::alloc(ssa_type, hints, ctx);

    for (unsigned i = 0; i < tuple_expr.exprs.size(); i++) {
        const sir::Expr &expr = tuple_expr.exprs[i];
        ssa::VirtualRegister ssa_field_ptr_reg = ctx.append_memberptr(stored_val.value_type, stored_val.get_ptr(), i);
        ssa::Value ssa_field_ptr = ssa::Value::from_register(ssa_field_ptr_reg, ssa::Primitive::ADDR);
        ExprSSAGenerator(ctx).generate_into_dst(expr, ssa_field_ptr);
    }

    return stored_val;
}

StoredValue ExprSSAGenerator::generate_coercion_expr(
    const sir::CoercionExpr &coercion_expr,
    const StorageHints &hints
) {
    ssa::Type ssa_type = TypeSSAGenerator(ctx).generate(coercion_expr.type);

    if (auto union_def = coercion_expr.type.match_symbol<sir::UnionDef>()) {
        sir::UnionCase &union_case = coercion_expr.value.get_type().as_symbol<sir::UnionCase>();
        StoredValue stored_val = StoredValue::alloc(ssa_type, hints, ctx);

        unsigned tag = union_def->get_index(union_case);
        ssa::VirtualRegister ssa_tag_ptr_reg = ctx.append_memberptr(stored_val.value_type, stored_val.get_ptr(), 0);
        ssa::VirtualRegister ssa_data_ptr_reg = ctx.append_memberptr(stored_val.value_type, stored_val.get_ptr(), 1);

        ctx.append_store(ssa::Operand::from_int_immediate(tag, ssa::Primitive::U32), ssa_tag_ptr_reg);
        ExprSSAGenerator(ctx).generate_into_dst(coercion_expr.value, ssa_data_ptr_reg);
        return stored_val;
    } else if (auto proto_def = coercion_expr.type.match_proto_ptr()) {
        const sir::Expr &base_type = coercion_expr.value.get_type().as<sir::PointerType>().base_type;
        const sir::StructDef &struct_def = base_type.as_symbol<sir::StructDef>();

        unsigned impl_index;

        for (unsigned i = 0; i < struct_def.impls.size(); i++) {
            if (proto_def == struct_def.impls[i].match_symbol<sir::ProtoDef>()) {
                impl_index = i;
            }
        }

        StoredValue stored_val = StoredValue::alloc(ssa_type, hints, ctx);

        ssa::VirtualRegister ssa_data_ptr_reg = ctx.append_memberptr(stored_val.value_type, stored_val.get_ptr(), 0);
        ExprSSAGenerator(ctx).generate_into_dst(coercion_expr.value, ssa_data_ptr_reg);

        ssa::VirtualRegister ssa_vtable_ptr_reg = ctx.append_memberptr(stored_val.value_type, stored_val.get_ptr(), 1);
        const std::vector<unsigned> &vtables = ctx.create_vtables(struct_def);

        if (!vtables.empty()) {
            unsigned vtable_global_index = vtables[impl_index];
            ssa::Global *vtable_global = ctx.ssa_mod->get_globals()[vtable_global_index];
            ctx.append_store(ssa::Operand::from_global(vtable_global, ssa::Primitive::ADDR), ssa_vtable_ptr_reg);
        } else {
            ctx.append_store(ssa::Operand::from_int_immediate(0, ssa::Primitive::ADDR), ssa_vtable_ptr_reg);
        }

        return StoredValue::create_value(stored_val.get_ptr());
    } else {
        return generate(coercion_expr.value, hints);
    }
}

StoredValue ExprSSAGenerator::generate_init_expr(const sir::InitExpr &init_expr, const StorageHints &hints) {
    sir::Ownership ownership = init_expr.resource->ownership;

    if (ownership == sir::Ownership::INIT_COND) {
        generate_deinit_flag_store(*init_expr.resource, DEINIT_FLAG_TRUE);
    }

    return generate(init_expr.value, hints);
}

StoredValue ExprSSAGenerator::generate_move_expr(const sir::MoveExpr &move_expr, const StorageHints &hints) {
    sir::Ownership ownership = move_expr.resource->ownership;

    if (ownership == sir::Ownership::MOVED_COND || ownership == sir::Ownership::INIT_COND) {
        generate_deinit_flag_store(*move_expr.resource, DEINIT_FLAG_FALSE);
    }

    return generate(move_expr.value, hints);
}

StoredValue ExprSSAGenerator::generate_deinit_expr(const sir::DeinitExpr &deinit_expr) {
    StoredValue ssa_val = generate_as_reference(deinit_expr.value);

    ctx.get_func_context().cur_deferred_deinits.push_back({
        .resource = deinit_expr.resource,
        .ssa_ptr = ssa_val.get_ptr(),
    });

    return ssa_val;
}

ssa::VirtualRegister ExprSSAGenerator::generate_pointer_expr(
    sir::BinaryOp op,
    ssa::Value ssa_lhs,
    ssa::Value ssa_rhs,
    ssa::Type ssa_base_type
) {
    if (op == sir::BinaryOp::ADD) {
        return ctx.append_offsetptr(std::move(ssa_lhs), std::move(ssa_rhs), ssa_base_type);
    } else if (op == sir::BinaryOp::SUB) {
        ssa::VirtualRegister ssa_neg_reg = ctx.next_vreg();
        ssa::Value ssa_zero = ssa::Value::from_int_immediate(0, ssa_rhs.get_type());
        ctx.get_ssa_block()->append(ssa::Instruction(ssa::Opcode::SUB, ssa_neg_reg, {ssa_zero, ssa_rhs}));

        ssa::Value ssa_neg_val = ssa::Value::from_register(ssa_neg_reg, ssa_rhs.get_type());
        return ctx.append_offsetptr(std::move(ssa_lhs), ssa_neg_val, ssa_base_type);
    } else {
        ASSERT_UNREACHABLE;
    }
}

StoredValue ExprSSAGenerator::generate_bool_expr(const sir::Expr &expr) {
    ssa::BasicBlockIter ssa_target_if_true = ctx.create_block();
    ssa::BasicBlockIter ssa_target_if_false = ctx.create_block();
    ssa::BasicBlockIter ssa_end = ctx.create_block();

    generate_branch(expr, {ssa_target_if_true, ssa_target_if_false});

    ssa::VirtualRegister ssa_slot_reg = ctx.append_alloca(ssa::Primitive::U8);
    ssa::Value ssa_slot = ssa::Value::from_register(ssa_slot_reg, ssa::Primitive::ADDR);

    ctx.append_block(ssa_target_if_true);
    ctx.append_store(ssa::Operand::from_int_immediate(1, ssa::Primitive::U8), ssa_slot);
    ctx.append_jmp(ssa_end);
    ctx.append_block(ssa_target_if_false);
    ctx.append_store(ssa::Operand::from_int_immediate(0, ssa::Primitive::U8), ssa_slot);
    ctx.append_jmp(ssa_end);
    ctx.append_block(ssa_end);

    return StoredValue::create_reference(ssa_slot, ssa::Primitive::U8);
}

void ExprSSAGenerator::generate_int_cmp_branch(
    const sir::BinaryExpr &binary_expr,
    ssa::Comparison ssa_cmp,
    CondBranchTargets targets
) {
    ssa::Value ssa_lhs = generate(binary_expr.lhs).turn_into_value(ctx).get_value();
    ssa::Value ssa_rhs = generate(binary_expr.rhs).turn_into_value(ctx).get_value();
    ctx.append_cjmp(ssa_lhs, ssa_cmp, ssa_rhs, targets.target_if_true, targets.target_if_false);
}

void ExprSSAGenerator::generate_fp_cmp_branch(
    const sir::BinaryExpr &binary_expr,
    ssa::Comparison ssa_cmp,
    CondBranchTargets targets
) {
    ssa::Value ssa_lhs = generate(binary_expr.lhs).turn_into_value(ctx).get_value();
    ssa::Value ssa_rhs = generate(binary_expr.rhs).turn_into_value(ctx).get_value();
    ctx.append_fcjmp(ssa_lhs, ssa_cmp, ssa_rhs, targets.target_if_true, targets.target_if_false);
}

void ExprSSAGenerator::generate_and_branch(const sir::BinaryExpr &binary_expr, CondBranchTargets targets) {
    ssa::BasicBlockIter ssa_rhs_block = ctx.create_block();
    generate_branch(binary_expr.lhs, {ssa_rhs_block, targets.target_if_false});
    ctx.append_block(ssa_rhs_block);
    generate_branch(binary_expr.rhs, {targets.target_if_true, targets.target_if_false});
}

void ExprSSAGenerator::generate_or_branch(const sir::BinaryExpr &binary_expr, CondBranchTargets targets) {
    ssa::BasicBlockIter ssa_rhs_block = ctx.create_block();
    generate_branch(binary_expr.lhs, {targets.target_if_true, ssa_rhs_block});
    ctx.append_block(ssa_rhs_block);
    generate_branch(binary_expr.rhs, {targets.target_if_true, targets.target_if_false});
}

void ExprSSAGenerator::generate_not_branch(const sir::UnaryExpr &unary_expr, CondBranchTargets targets) {
    generate_branch(unary_expr.value, {targets.target_if_false, targets.target_if_true});
}

void ExprSSAGenerator::generate_zero_check_branch(const sir::Expr &expr, CondBranchTargets targets) {
    ssa::Value ssa_value = generate(expr).turn_into_value(ctx).get_value();
    ssa::Comparison ssa_cmp = ssa::Comparison::NE;
    ssa::Value ssa_zero = ssa::Value::from_int_immediate(0, ssa_value.get_type());
    ctx.append_cjmp(ssa_value, ssa_cmp, ssa_zero, targets.target_if_true, targets.target_if_false);
}

void ExprSSAGenerator::generate_deinit_flag_store(const sir::Resource &resource, const ssa::Value &value) {
    ssa::VirtualRegister deinit_flag = ctx.get_func_context().resource_deinit_flags.at(&resource);
    ctx.append_store(value, deinit_flag);

    for (const sir::Resource &sub_resource : resource.sub_resources) {
        generate_deinit_flag_store(sub_resource, value);
    }
}

ssa::Value ExprSSAGenerator::generate_field_access(
    const StoredValue &base,
    unsigned field_index,
    sir::Attributes::Layout type_layout
) {
    if (type_layout == sir::Attributes::Layout::DEFAULT) {
        ssa::VirtualRegister ssa_field_ptr_reg = ctx.append_memberptr(base.value_type, base.get_ptr(), field_index);
        return ssa::Value::from_register(ssa_field_ptr_reg, ssa::Primitive::ADDR);
    } else if (type_layout == sir::Attributes::Layout::OVERLAPPING) {
        return base.get_ptr();
    } else {
        ASSERT_UNREACHABLE;
    }
}

sir::Attributes::Layout ExprSSAGenerator::get_type_layout(const sir::Expr &type) {
    if (auto struct_def = type.match_symbol<sir::StructDef>()) {
        return struct_def->get_layout();
    } else {
        return sir::Attributes::Layout::DEFAULT;
    }
}

} // namespace lang

} // namespace banjo
