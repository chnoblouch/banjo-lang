#include "expr_ir_builder.hpp"

#include "ast/ast_child_indices.hpp"
#include "ast/ast_node.hpp"
#include "ir/structure.hpp"
#include "ir/virtual_register.hpp"
#include "ir_builder/bool_expr_ir_builder.hpp"
#include "ir_builder/closure_ir_builder.hpp"
#include "ir_builder/conversion.hpp"
#include "ir_builder/func_call_ir_builder.hpp"
#include "ir_builder/ir_builder_utils.hpp"
#include "ir_builder/location_ir_builder.hpp"
#include "ir_builder/storage.hpp"
#include "symbol/data_type.hpp"
#include "symbol/standard_types.hpp"
#include "utils/macros.hpp"

#include <optional>
#include <utility>

namespace ir_builder {

ExprIRBuilder::ExprIRBuilder(IRBuilderContext &context, lang::ASTNode *node)
  : IRBuilder(context, node),
    lang_type(node->as<lang::Expr>()->get_data_type()) {}

void ExprIRBuilder::set_coercion_level(unsigned coercion_level) {
    this->coercion_level = {coercion_level};
    lang_type = node->as<lang::Expr>()->get_coercion_chain()[coercion_level];
}

StoredValue ExprIRBuilder::build() {
    return build_node(node, StorageHints::NONE);
}

StoredValue ExprIRBuilder::build_into_value() {
    return build_node(node, StorageHints::NONE).turn_into_value(context);
}

StoredValue ExprIRBuilder::build_into_value_if_possible() {
    return build_node(node, StorageHints::NONE).try_turn_into_value(context);
}

StoredValue ExprIRBuilder::build_into_ptr() {
    return build_node(node, StorageHints::PREFER_REFERENCE).turn_into_reference(context);
}

void ExprIRBuilder::build_and_store(const ir::Value &dst) {
    build_node(node, dst).copy_to(dst, context);
}

void ExprIRBuilder::build_and_store(ir::VirtualRegister dst) {
    build_and_store(ir::Value::from_register(dst));
}

StoredValue ExprIRBuilder::build_node(lang::ASTNode *node, const StorageHints &hints) {
    lang::Expr *expr = node->as<lang::Expr>();

    if (is_coerced()) {
        lang::DataType *coercion_base = get_coercion_base();

        if (lang::StandardTypes::is_optional(lang_type)) {
            return build_implicit_optional(expr);
        } else if (lang::StandardTypes::is_result(lang_type)) {
            return build_implicit_result(expr);
        } else if (lang_type->get_kind() == lang::DataType::Kind::UNION &&
                   coercion_base->get_kind() == lang::DataType::Kind::UNION_CASE) {
            ir::Structure *struct_ = lang_type->get_union()->get_ir_struct();
            StoredValue stored_val = StoredValue::alloc(struct_, hints, context);

            unsigned tag = lang_type->get_union()->get_case_index(coercion_base->get_union_case());
            ir::VirtualRegister tag_ptr_reg = context.append_memberptr(stored_val.value_type, stored_val.get_ptr(), 0);
            ir::Value tag_ptr = ir::Value::from_register(tag_ptr_reg, ir::Primitive::I32);
            context.append_store(ir::Operand::from_int_immediate(tag, ir::Primitive::I32), tag_ptr);

            ir::VirtualRegister data_ptr_reg = context.append_memberptr(stored_val.value_type, stored_val.get_ptr(), 1);
            ir::Value data_ptr = ir::Value::from_register(data_ptr_reg, ir::Primitive::ADDR);

            ExprIRBuilder data_builder(context, node);
            data_builder.set_coercion_level(coercion_level + 1);
            data_builder.build_and_store(data_ptr);

            return stored_val;
        }
    }

    switch (node->get_type()) {
        case lang::AST_INT_LITERAL: return build_int_literal(node);
        case lang::AST_FLOAT_LITERAL: return build_float_literal(node);
        case lang::AST_CHAR_LITERAL: return build_char_literal(node);
        case lang::AST_STRING_LITERAL: return build_string_literal(node);
        case lang::AST_ARRAY_EXPR: return build_array_literal(node, hints);
        case lang::AST_MAP_EXPR: return build_map_literal(node);
        case lang::AST_TUPLE_EXPR: return build_tuple_literal(node, hints);
        case lang::AST_CLOSURE: return ClosureIRBuilder(context, node).build(hints);
        case lang::AST_FALSE: return create_immediate(0, ir::Primitive::I8);
        case lang::AST_TRUE: return create_immediate(1, ir::Primitive::I8);
        case lang::AST_NULL: return create_immediate(0, ir::Primitive::ADDR);
        case lang::AST_OPERATOR_EQ:
        case lang::AST_OPERATOR_NE:
        case lang::AST_OPERATOR_GT:
        case lang::AST_OPERATOR_LT:
        case lang::AST_OPERATOR_GE:
        case lang::AST_OPERATOR_LE:
        case lang::AST_OPERATOR_AND:
        case lang::AST_OPERATOR_OR: return build_bool_expr(node, hints);
        case lang::AST_OPERATOR_ADD:
        case lang::AST_OPERATOR_SUB:
        case lang::AST_OPERATOR_MUL:
        case lang::AST_OPERATOR_DIV:
        case lang::AST_OPERATOR_MOD:
        case lang::AST_OPERATOR_BIT_AND:
        case lang::AST_OPERATOR_BIT_OR:
        case lang::AST_OPERATOR_BIT_XOR:
        case lang::AST_OPERATOR_SHL:
        case lang::AST_OPERATOR_SHR: return build_binary_operation(node, hints);
        case lang::AST_OPERATOR_NEG: return build_neg(node);
        case lang::AST_OPERATOR_REF: return build_ref(node);
        case lang::AST_STAR_EXPR: return build_deref(node->as<lang::Expr>());
        case lang::AST_OPERATOR_NOT: return build_not(node, hints);
        case lang::AST_IDENTIFIER:
        case lang::AST_DOT_OPERATOR:
        case lang::AST_SELF: return build_location(node);
        case lang::AST_FUNCTION_CALL: return build_call(node, hints);
        case lang::AST_ARRAY_ACCESS: return build_bracket_expr(node->as<lang::BracketExpr>());
        case lang::AST_CAST: return build_cast(node);
        case lang::AST_STRUCT_INSTANTIATION: return build_struct_literal(node, lang::STRUCT_LITERAL_VALUES, hints);
        case lang::AST_ANON_STRUCT_LITERAL: return build_struct_literal(node, 0, hints);
        case lang::AST_META_EXPR: return build_meta_expr(node);
        case lang::AST_META_FIELD_ACCESS:
        case lang::AST_META_METHOD_CALL: return build_node(node->as<lang::MetaExpr>()->get_value(), hints);
        default: ASSERT_UNREACHABLE;
    }
}

StoredValue ExprIRBuilder::build_int_literal(lang::ASTNode *node) {
    const LargeInt &value = node->as<lang::IntLiteral>()->get_value();
    ir::Type type = IRBuilderUtils::build_type(lang_type);
    ir::Value immediate = ir::Value::from_int_immediate(value, type);

    return StoredValue::create_value(immediate);
}

StoredValue ExprIRBuilder::build_float_literal(lang::ASTNode *node) {
    double value = std::stod(node->get_value());
    ir::Type type = IRBuilderUtils::build_type(lang_type);
    ir::Value immediate = ir::Value::from_fp_immediate(value, type);

    return StoredValue::create_value(immediate);
}

StoredValue ExprIRBuilder::build_char_literal(lang::ASTNode *node) {
    unsigned index = 0;
    char encoded_val = encode_char(node->get_value(), index);
    ir::Value immediate = ir::Value::from_int_immediate(encoded_val, ir::Primitive::I8);
    return StoredValue::create_value(immediate);
}

StoredValue ExprIRBuilder::build_string_literal(lang::ASTNode *node) {
    StoredValue cstr = build_cstr_string_literal(node->get_value());

    if (is_coerced() && lang::StandardTypes::is_string(lang_type)) {
        lang::Structure *lang_struct = lang_type->get_structure();
        ir::Type struct_type(lang_struct->get_ir_struct());

        lang::DataType u8(lang::PrimitiveType::U8);
        lang::DataType u8_ptr;
        u8_ptr.set_to_pointer(&u8);

        lang::Function *new_func = lang_struct->get_symbol_table()->get_function_with_params("from", {&u8_ptr});
        return IRBuilderUtils::build_call(new_func, {cstr.value_or_ptr}, context);
    } else {
        return cstr;
    }
}

StoredValue ExprIRBuilder::build_array_literal(lang::ASTNode *node, const StorageHints &hints) {
    if (lang_type->get_kind() == lang::DataType::Kind::STATIC_ARRAY) {
        return build_static_array_literal(node, hints);
    } else {
        return build_dynamic_array_literal(node, hints);
    }
}

StoredValue ExprIRBuilder::build_dynamic_array_literal(lang::ASTNode *node, const StorageHints &hints) {
    unsigned num_elements = node->get_children().size();

    lang::Structure *lang_struct = lang_type->get_structure();
    ir::Type struct_type(lang_struct->get_ir_struct());

    lang::Function *create_func = lang_struct->get_symbol_table()->get_symbol("sized")->get_func();
    std::vector<ir::Value> args = {ir::Value::from_int_immediate(num_elements, ir::Primitive::I32)};

    StoredValue stored_val;
    if (hints.dst) {
        stored_val = IRBuilderUtils::build_call({create_func, args}, *hints.dst, context);
    } else {
        stored_val = IRBuilderUtils::build_call({create_func, args}, context);
    }

    ir::Operand val_ptr = stored_val.value_or_ptr;

    lang::Function *set_func = lang_struct->get_method_table().get_function("set");
    for (unsigned i = 0; i < num_elements; i++) {
        lang::ASTNode *child = node->get_child(i);

        ir::Value index = ir::Value::from_int_immediate(i, ir::Primitive::I32);
        ir::Value element = IRBuilderUtils::build_arg(child, context);
        IRBuilderUtils::build_call(set_func, {val_ptr, index, element}, context);
    }

    return stored_val;
}

StoredValue ExprIRBuilder::build_static_array_literal(lang::ASTNode *node, const StorageHints &hints) {
    ir::Type type = IRBuilderUtils::build_type(lang_type);
    StoredValue stored_val = StoredValue::alloc(type, hints, context);
    ir::Value val_ptr = stored_val.get_ptr();

    ir::Type base_type = IRBuilderUtils::build_type(lang_type->get_static_array_type().base_type);

    for (unsigned i = 0; i < node->get_children().size(); i++) {
        lang::ASTNode *element_node = node->get_child(i);
        ir::VirtualRegister element_ptr_reg = context.append_offsetptr(val_ptr, i, base_type);
        ExprIRBuilder(context, element_node).build_and_store(element_ptr_reg);
    }

    return stored_val;
}

StoredValue ExprIRBuilder::build_map_literal(lang::ASTNode *node) {
    lang::Structure *lang_struct = lang_type->get_structure();
    ir::Type struct_type(lang_struct->get_ir_struct());

    lang::Function *new_func = lang_struct->get_symbol_table()->get_function("new");
    StoredValue stored_val = IRBuilderUtils::build_call(new_func, {}, context);
    ir::Operand val_ptr = stored_val.value_or_ptr;

    lang::Function *insert_func = lang_struct->get_method_table().get_function("insert");
    for (unsigned int i = 0; i < node->get_children().size(); i++) {
        lang::ASTNode *child = node->get_child(i);
        lang::ASTNode *key_node = child->get_child(lang::MAP_LITERAL_PAIR_KEY);
        lang::ASTNode *value_node = child->get_child(lang::MAP_LITERAL_PAIR_VALUE);

        ir::Value key = IRBuilderUtils::build_arg(key_node, context);
        ir::Value value = IRBuilderUtils::build_arg(value_node, context);
        IRBuilderUtils::build_call(insert_func, {val_ptr, key, value}, context);
    }

    return stored_val;
}

StoredValue ExprIRBuilder::build_struct_literal(lang::ASTNode *node, unsigned values_index, const StorageHints &hints) {
    lang::ASTNode *values_node = node->get_child(values_index);

    ir::Structure *struct_ = lang_type->get_structure()->get_ir_struct();
    StoredValue stored_val = StoredValue::alloc(ir::Type(struct_), hints, context);
    const ir::Value &val_ptr = stored_val.get_ptr();
    const ir::Type &val_type = stored_val.value_type;

    for (lang::ASTNode *field_val_node : values_node->get_children()) {
        lang::ASTNode *field_name_node = field_val_node->get_child(0);
        lang::ASTNode *field_value_node = field_val_node->get_child(1);

        if (field_val_node->get_child(1)->get_type() == lang::AST_UNDEFINED) {
            continue;
        }

        const std::string &field_name = field_name_node->get_value();
        unsigned field_offset = struct_->get_member_index(field_name);
        ir::VirtualRegister field_reg = context.append_memberptr(val_type, val_ptr, field_offset);

        ExprIRBuilder(context, field_value_node).build_and_store(field_reg);
    }

    return stored_val;
}

StoredValue ExprIRBuilder::build_tuple_literal(lang::ASTNode *node, const StorageHints &hints) {
    ir::Type type = IRBuilderUtils::build_type(lang_type);

    StoredValue stored_val = StoredValue::alloc(type, hints, context);
    const ir::Value &val_ptr = stored_val.get_ptr();
    const ir::Type &val_type = stored_val.value_type;

    for (unsigned i = 0; i < node->get_children().size(); i++) {
        lang::ASTNode *child = node->get_child(i);
        ir::Type member_type = type.get_tuple_types()[i];
        ir::VirtualRegister offset_ptr_reg = context.append_memberptr(val_type, val_ptr, i);
        ExprIRBuilder(context, child).build_and_store(offset_ptr_reg);
    }

    return stored_val;
}

StoredValue ExprIRBuilder::build_binary_operation(lang::ASTNode *node, const StorageHints &hints) {
    if (is_overloaded_operator_call(node)) {
        return build_overloaded_operator_call(node, hints);
    }

    lang::Expr *lhs = node->get_child(0)->as<lang::Expr>();
    lang::Expr *rhs = node->get_child(1)->as<lang::Expr>();
    lang::ASTNodeType op = node->get_type();

    bool is_signed = lhs->get_data_type()->is_signed_int();

    ir::Value lhs_val = ExprIRBuilder(context, lhs).build_into_value().get_value();
    ir::Value rhs_val = ExprIRBuilder(context, rhs).build_into_value().get_value();

    if (lhs->get_data_type()->get_kind() == lang::DataType::Kind::POINTER) {
        assert(op == lang::AST_OPERATOR_ADD);
        ir::Type base_type = IRBuilderUtils::build_type(lhs->get_data_type()->get_base_data_type());
        ir::VirtualRegister reg = context.append_offsetptr(lhs_val, rhs_val, base_type);
        return StoredValue::create_value(reg, lhs_val.get_type());
    }

    ir::VirtualRegister reg = context.get_current_func()->next_virtual_reg();

    ir::Opcode opcode;
    bool commutative = false;

    if (!lhs_val.get_type().is_floating_point()) {
        switch (op) {
            case lang::AST_OPERATOR_ADD:
                opcode = ir::Opcode::ADD;
                commutative = true;
                break;
            case lang::AST_OPERATOR_SUB: opcode = ir::Opcode::SUB; break;
            case lang::AST_OPERATOR_MUL:
                opcode = ir::Opcode::MUL;
                commutative = true;
                break;
            case lang::AST_OPERATOR_DIV: opcode = is_signed ? ir::Opcode::SDIV : ir::Opcode::UDIV; break;
            case lang::AST_OPERATOR_MOD: opcode = is_signed ? ir::Opcode::SREM : ir::Opcode::UREM; break;
            case lang::AST_OPERATOR_BIT_AND:
                opcode = ir::Opcode::AND;
                commutative = true;
                break;
            case lang::AST_OPERATOR_BIT_OR:
                opcode = ir::Opcode::OR;
                commutative = true;
                break;
            case lang::AST_OPERATOR_BIT_XOR:
                opcode = ir::Opcode::XOR;
                commutative = true;
                break;
            case lang::AST_OPERATOR_SHL: opcode = ir::Opcode::SHL; break;
            case lang::AST_OPERATOR_SHR: opcode = ir::Opcode::SHR; break;
            default: ASSERT_UNREACHABLE;
        }
    } else {
        switch (op) {
            case lang::AST_OPERATOR_ADD: opcode = ir::Opcode::FADD; break;
            case lang::AST_OPERATOR_SUB: opcode = ir::Opcode::FSUB; break;
            case lang::AST_OPERATOR_MUL: opcode = ir::Opcode::FMUL; break;
            case lang::AST_OPERATOR_DIV: opcode = ir::Opcode::FDIV; break;
            default: ASSERT_UNREACHABLE;
        }
    }

    // Optimization becomes easier if immediates are always on the right-hand side.
    if (commutative && lhs_val.is_immediate()) {
        std::swap(lhs_val, rhs_val);
    }

    if (opcode == ir::Opcode::SHL || opcode == ir::Opcode::SHR) {
        rhs_val.set_type(ir::Primitive::I8);
    }

    context.get_cur_block().append(ir::Instruction(opcode, reg, {lhs_val, rhs_val}));
    return StoredValue::create_value(reg, lhs_val.get_type());
}

StoredValue ExprIRBuilder::build_overloaded_operator_call(lang::ASTNode *node, const StorageHints &hints) {
    lang::ASTNode *lhs = node->get_child(0);
    lang::ASTNode *rhs = node->get_child(1);
    lang::Function *func = node->as<lang::OperatorExpr>()->get_operator_func();

    ir::Value lhs_val = ExprIRBuilder(context, lhs).build_into_ptr().get_ptr();
    ir::Value rhs_val;

    if (func->get_type().param_types[1]->get_kind() == lang::DataType::Kind::POINTER) {
        rhs_val = ExprIRBuilder(context, rhs).build_into_ptr().get_ptr();
    } else {
        rhs_val = ExprIRBuilder(context, rhs).build_into_value_if_possible().value_or_ptr;
    }

    IRBuilderUtils::FuncCall call{
        .func = func,
        .args = {lhs_val, rhs_val},
    };

    ir::Value dst;
    if (hints.dst) {
        dst = *hints.dst;
    } else if (func->is_return_by_ref()) {
        ir::Type type = IRBuilderUtils::build_type(func->get_type().return_type);
        ir::VirtualRegister dst_reg = context.append_alloca(type);
        dst = ir::Value::from_register(dst_reg, ir::Primitive::VOID);
    } else {
        ir::VirtualRegister dst_reg = context.get_current_func()->next_virtual_reg();
        dst = ir::Value::from_register(dst_reg, ir::Primitive::VOID);
    }

    return IRBuilderUtils::build_call(call, dst, context);
}

StoredValue ExprIRBuilder::build_neg(lang::ASTNode *node) {
    ir::Value value_to_negate = ExprIRBuilder(context, node->get_child()).build_into_value().get_value();
    ir::VirtualRegister dst = context.get_current_func()->next_virtual_reg();
    ir::Type type = value_to_negate.get_type();

    if (!type.is_floating_point()) {
        ir::Operand zero = ir::Operand::from_int_immediate(0, type);
        context.get_cur_block().append(ir::Instruction(ir::Opcode::SUB, dst, {zero, value_to_negate}));
    } else {
        ir::Operand zero = ir::Operand::from_fp_immediate(0.0, type);
        context.get_cur_block().append(ir::Instruction(ir::Opcode::FSUB, dst, {zero, value_to_negate}));
    }

    return StoredValue::create_value(dst, type);
}

StoredValue ExprIRBuilder::build_ref(lang::ASTNode *node) {
    StoredValue stored_val = build_node(node->get_child(), StorageHints::PREFER_REFERENCE);
    return StoredValue::create_value(stored_val.get_ptr());
}

StoredValue ExprIRBuilder::build_deref(lang::Expr *node) {
    lang::Expr *child_node = node->get_child()->as<lang::Expr>();

    ExprIRBuilder child_builder(context, node->get_child());
    StoredValue stored_val = child_builder.build().try_turn_into_value(context);

    lang::DataType *child_lang_type = child_node->get_data_type();

    if (child_lang_type->get_kind() == lang::DataType::Kind::POINTER) {
        ir::Type type = IRBuilderUtils::build_type(node->get_data_type());
        return StoredValue::create_reference(stored_val.value_or_ptr, type);
    } else if (child_lang_type->get_kind() == lang::DataType::Kind::STRUCT) {
        lang::Structure *struct_ = child_lang_type->get_structure();
        lang::Function *func = struct_->get_method_table().get_function("deref");
        return IRBuilderUtils::build_call(func, {stored_val.value_or_ptr}, context);
    } else {
        ASSERT_UNREACHABLE;
    }
}

StoredValue ExprIRBuilder::build_not(lang::ASTNode *node, const StorageHints &hints) {
    return build_bool_expr(node, hints);
}

StoredValue ExprIRBuilder::build_location(lang::ASTNode *node) {
    return LocationIRBuilder(context, node).build(true);
}

StoredValue ExprIRBuilder::build_call(lang::ASTNode *node, const StorageHints &hints) {
    lang::ASTNode *location_node = node->get_child(lang::CALL_LOCATION);

    const std::optional<lang::Location> &location = location_node->as<lang::Expr>()->get_location();
    if (location && location->get_last_element().is_union_case()) {
        return build_union_case_expr(node, hints);
    } else {
        return build_func_call(node, hints);
    }
}

StoredValue ExprIRBuilder::build_union_case_expr(lang::ASTNode *node, const StorageHints &hints) {
    lang::ASTNode *location_node = node->get_child(lang::CALL_LOCATION);
    lang::ASTNode *args_node = node->get_child(lang::CALL_ARGS);

    lang::UnionCase *lang_case = location_node->as<lang::Expr>()->get_location()->get_last_element().get_union_case();
    ir::Structure *struct_ = lang_case->get_ir_struct();

    StoredValue stored_val = StoredValue::alloc(struct_, hints, context);
    const ir::Value &val_ptr = stored_val.get_ptr();
    const ir::Type &val_type = stored_val.value_type;

    for (unsigned i = 0; i < args_node->get_children().size(); i++) {
        lang::ASTNode *arg_node = args_node->get_child(i);
        if (arg_node->get_type() == lang::AST_UNDEFINED) {
            continue;
        }

        ir::VirtualRegister field_reg = context.append_memberptr(val_type, val_ptr, i);
        ExprIRBuilder(context, arg_node).build_and_store(field_reg);
    }

    return stored_val;
}

StoredValue ExprIRBuilder::build_func_call(lang::ASTNode *node, const StorageHints &hints) {
    FuncCallIRBuilder func_call_builder(context, node);
    ir::VirtualRegister dst = func_call_builder.build(hints.dst ? *hints.dst : StorageHints::NONE, true);

    ir::Type type = IRBuilderUtils::build_type(lang_type);

    if (!is_return_by_ref(type)) {
        return StoredValue::create_value(dst, type);
    } else {
        return StoredValue::create_reference(dst, type);
    }
}

StoredValue ExprIRBuilder::build_bracket_expr(lang::BracketExpr *node) {
    assert(node->get_kind() == lang::BracketExpr::Kind::INDEX);

    lang::Expr *base_node = node->get_child(0)->as<lang::Expr>();
    lang::Expr *index_node = node->get_child(1)->get_child(0)->as<lang::Expr>();

    lang::DataType *lang_type = base_node->get_data_type();

    if (lang_type->get_kind() == lang::DataType::Kind::POINTER) {
        StoredValue built_pointer = ExprIRBuilder(context, base_node).build().try_turn_into_value(context);
        ir::Type base_type = IRBuilderUtils::build_type(lang_type->get_base_data_type());
        StoredValue built_index = ExprIRBuilder(context, index_node).build().try_turn_into_value(context);
        return build_offset_ptr(built_pointer, built_index, base_type);
    } else if (lang_type->get_kind() == lang::DataType::Kind::STATIC_ARRAY) {
        StoredValue built_array = build_node(base_node, StorageHints::PREFER_REFERENCE);
        ir::Type base_type = IRBuilderUtils::build_type(lang_type->get_static_array_type().base_type);
        StoredValue built_index = ExprIRBuilder(context, index_node).build().try_turn_into_value(context);
        return build_offset_ptr(built_array, built_index, base_type);
    } else if (lang_type->get_kind() == lang::DataType::Kind::STRUCT) {
        StoredValue built_array = ExprIRBuilder(context, base_node).build().turn_into_reference(context);
        StoredValue built_index = ExprIRBuilder(context, index_node).build().try_turn_into_value(context);

        lang::Structure *struct_ = base_node->as<lang::Expr>()->get_data_type()->get_structure();
        lang::Function *ref_func = struct_->get_method_table().get_function("ref");

        std::vector<ir::Value> args{built_array.get_ptr(), built_index.value_or_ptr};
        StoredValue pointer = IRBuilderUtils::build_call(ref_func, args, context);
        ir::Type type = IRBuilderUtils::build_type(ref_func->get_type().return_type->get_base_data_type());
        return StoredValue::create_reference(pointer.get_value(), type);
    } else {
        ASSERT_UNREACHABLE;
    }
}

StoredValue ExprIRBuilder::build_cast(lang::ASTNode *node) {
    lang::DataType *old_lang_type = node->get_child(0)->as<lang::Expr>()->get_data_type();
    lang::DataType *new_lang_type = node->as<lang::Expr>()->get_data_type();

    ir::Value old_val = ExprIRBuilder(context, node->get_child(0)).build_into_value().get_value();
    ir::Value new_val = Conversion::build(context, old_val, old_lang_type, new_lang_type);
    return StoredValue::create_value(new_val);
}

StoredValue ExprIRBuilder::build_meta_expr(lang::ASTNode *node) {
    lang::ASTNode *args_node = node->get_child(lang::META_EXPR_ARGS);
    lang::ASTNode *type_node = args_node->get_child(0);

    ir::Type type = IRBuilderUtils::build_type(type_node->as<lang::Expr>()->get_data_type());
    int size = get_size(type);

    ir::Value immediate = ir::Value::from_int_immediate(size, ir::Primitive::I64);
    return StoredValue::create_value(immediate);
}

StoredValue ExprIRBuilder::build_bool_expr(lang::ASTNode *node, const StorageHints &hints) {
    if (is_overloaded_operator_call(node)) {
        return build_overloaded_operator_call(node, hints);
    }

    int label_id = context.next_cmp_to_val_id();

    ir::BasicBlockIter true_block = context.create_block("cmp." + std::to_string(label_id) + ".true");
    ir::BasicBlockIter false_block = context.create_block("cmp." + std::to_string(label_id) + ".false");
    ir::BasicBlockIter end_block = context.create_block("cmp." + std::to_string(label_id) + ".end");

    ir::VirtualRegister reg = context.get_current_func()->next_virtual_reg();
    context.append_alloca(reg, ir::Primitive::I8);

    BoolExprIRBuilder(context, node, true_block, false_block).build();

    ir::Operand dst = ir::Operand::from_register(reg, ir::Primitive::ADDR);
    context.append_block(true_block);
    context.append_store(ir::Operand::from_int_immediate(1, ir::Primitive::I8), dst);
    context.append_jmp(end_block);
    context.append_block(false_block);
    context.append_store(ir::Operand::from_int_immediate(0, ir::Primitive::I8), dst);
    context.append_jmp(end_block);
    context.append_block(end_block);

    return StoredValue::create_reference(reg, ir::Primitive::I8);
}

StoredValue ExprIRBuilder::build_implicit_optional(lang::Expr *expr) {
    lang::Structure *lang_struct = lang_type->get_structure();
    ir::Type struct_type(lang_struct->get_ir_struct());

    if (expr->get_type() != lang::AST_NONE) {
        ExprIRBuilder val_builder(context, expr);
        val_builder.set_coercion_level(coercion_level + 1);
        ir::Value val = val_builder.build_into_value_if_possible().value_or_ptr;

        lang::Function *some_func = lang_struct->get_symbol_table()->get_function("new_some");
        return IRBuilderUtils::build_call(some_func, {val}, context);
    } else {
        lang::Function *none_func = lang_struct->get_symbol_table()->get_function("new_none");
        return IRBuilderUtils::build_call(none_func, {}, context);
    }
}

StoredValue ExprIRBuilder::build_implicit_result(lang::Expr *expr) {
    lang::Structure *lang_struct = lang_type->get_structure();
    ir::Type struct_type(lang_struct->get_ir_struct());

    lang::DataType *error_lang_type = lang::StandardTypes::get_result_error_type(lang_type);
    lang::DataType *coercion_from = expr->get_coercion_chain()[coercion_level + 1];
    bool is_error = coercion_from->equals(error_lang_type);

    const std::string &func_name = is_error ? "failure" : "success";
    lang::Function *func = lang_struct->get_symbol_table()->get_function(func_name);

    ExprIRBuilder val_builder(context, expr);
    val_builder.set_coercion_level(coercion_level + 1);
    ir::Value val = val_builder.build_into_value_if_possible().value_or_ptr;

    return IRBuilderUtils::build_call(func, {val}, context);
}

char ExprIRBuilder::encode_char(const std::string &value, unsigned &index) {
    char c = value[index++];

    if (c == '\\') {
        c = value[index++];

        if (c == 'n') return 0x0A;
        else if (c == 'r') return 0x0D;
        else if (c == 't') return 0x09;
        else if (c == '0') return 0x00;
        else if (c == '\\') return '\\';
        else if (c == 'x') {
            index += 2;
            return (char)std::stoi(value.substr(index - 2, 2), nullptr, 16);
        }
    }

    return c;
}

StoredValue ExprIRBuilder::build_cstr_string_literal(const std::string &value) {
    unsigned index = 0;
    std::string encoded_val = "";

    while (index < value.length()) {
        encoded_val += encode_char(value, index);
    }
    encoded_val += '\0';

    std::string name = context.next_string_name();
    ir::Type type = ir::Primitive::ADDR;
    context.get_current_mod()->add(ir::Global(name, type, ir::Operand::from_string(encoded_val)));
    ir::Value global_val = ir::Operand::from_global(name, type);
    return StoredValue::create_value(global_val);
}

StoredValue ExprIRBuilder::build_offset_ptr(const StoredValue &pointer, const StoredValue &index, ir::Type base_type) {
    ir::Type offset_type = context.get_target()->get_data_layout().get_usize_type();
    ir::Operand offset;

    if (index.value_or_ptr.is_immediate()) {
        offset = ir::Operand::from_int_immediate(index.get_value().get_int_immediate(), offset_type);
    } else if (index.value_or_ptr.is_register()) {
        // FIXME: extend to usize if necessary
        offset = ir::Operand::from_register(index.get_value().get_register(), offset_type);
    }

    ir::VirtualRegister offset_ptr = context.append_offsetptr(pointer.value_or_ptr, offset, base_type);
    return StoredValue::create_reference(offset_ptr, std::move(base_type));
}

StoredValue ExprIRBuilder::create_immediate(LargeInt value, ir::Type type) {
    return StoredValue::create_value(ir::Value::from_int_immediate(value, std::move(type)));
}

bool ExprIRBuilder::is_coerced() {
    lang::Expr *expr = node->as<lang::Expr>();
    unsigned next_coercion_level = coercion_level + 1;
    return expr->is_coerced() && expr->get_coercion_chain().size() - 1 >= next_coercion_level;
}

lang::DataType *ExprIRBuilder::get_coercion_base() {
    lang::Expr *expr = node->as<lang::Expr>();
    return expr->get_coercion_chain()[coercion_level + 1];
}

bool ExprIRBuilder::is_overloaded_operator_call(lang::ASTNode *node) {
    if (node->get_children().size() < 2) {
        return false;
    }

    lang::Expr *lhs = node->get_child(0)->as<lang::Expr>();
    return lhs->get_data_type()->get_kind() == lang::DataType::Kind::STRUCT;
}

} // namespace ir_builder
