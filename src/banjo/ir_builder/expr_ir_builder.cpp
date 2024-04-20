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
#include "symbol/magic_functions.hpp"
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

StoredValue ExprIRBuilder::build(StorageReqs reqs) {
    return build_node(node, reqs);
}

StoredValue ExprIRBuilder::build_into_stored_val_if_possible(StorageReqs reqs /* = {} */) {
    StoredValue stored_val = build(reqs);
    if (!stored_val.reference || !stored_val.fits_in_reg) {
        return stored_val;
    }

    ir::VirtualRegister reg = context.append_load(stored_val.value_or_ptr);
    ir::Value val = ir::Value::from_register(reg, stored_val.value_or_ptr.get_type().deref());
    return StoredValue::create_value(val, context);
}

ir::Value ExprIRBuilder::build_into_value() {
    return build_into_value(node);
}

ir::Value ExprIRBuilder::build_into_value_if_possible() {
    StoredValue stored_val = build({});

    ir::Value val = stored_val.value_or_ptr;
    if (!stored_val.reference || !stored_val.fits_in_reg) {
        return val;
    }

    ir::VirtualRegister reg = context.append_load(val);
    return ir::Value::from_register(reg, val.get_type().deref());
}

ir::Value ExprIRBuilder::build_into_ptr() {
    StoredValue stored_val = build(StorageReqs::FORCE_REFERENCE);

    if (stored_val.reference) {
        return stored_val.value_or_ptr;
    } else {
        ir::VirtualRegister dst_reg = context.append_alloca(stored_val.value_or_ptr.get_type());
        ir::Value dst = ir::Value::from_register(dst_reg, stored_val.value_or_ptr.get_type().ref());
        context.append_store(stored_val.value_or_ptr, dst);
        return dst;
    }
}

void ExprIRBuilder::build_and_store(StorageReqs reqs) {
    StoredValue stored_val = build_into_stored_val_if_possible(reqs);
    if (stored_val.value_or_ptr.is_register() && stored_val.value_or_ptr == reqs.dst) {
        return;
    }

    if (!stored_val.reference) {
        context.append_store(stored_val.value_or_ptr, *reqs.dst);
    } else {
        const ir::Value &src = stored_val.value_or_ptr;
        unsigned size = get_size(src.get_type().deref());

        ir::Value dst = *reqs.dst;
        dst.set_type(src.get_type());

        context.append_copy(dst, src, size);
    }
}

ir::Value ExprIRBuilder::build_into_value(lang::ASTNode *node) {
    StoredValue stored_val = build_node(node, {});

    ir::Value val = stored_val.value_or_ptr;
    if (!stored_val.reference) {
        return val;
    }

    assert(stored_val.fits_in_reg);
    ir::VirtualRegister reg = context.append_load(val);
    return ir::Value::from_register(reg, val.get_type().deref());
}

StoredValue ExprIRBuilder::build_node(lang::ASTNode *node, StorageReqs reqs) {
    lang::Expr *expr = node->as<lang::Expr>();

    if (is_coerced()) {
        lang::DataType *coercion_base = get_coercion_base();

        if (lang::StandardTypes::is_optional(lang_type)) {
            return build_implicit_optional(expr, reqs);
        } else if (lang::StandardTypes::is_result(lang_type)) {
            return build_implicit_result(expr, reqs);
        } else if (lang_type->get_kind() == lang::DataType::Kind::UNION &&
                   coercion_base->get_kind() == lang::DataType::Kind::UNION_CASE) {
            ir::Structure *struct_ = lang_type->get_union()->get_ir_struct();
            StoredValue stored_val = StoredValue::alloc(struct_, reqs, context);

            unsigned tag = lang_type->get_union()->get_case_index(coercion_base->get_union_case());
            ir::VirtualRegister tag_ptr_reg = context.append_memberptr(stored_val.value_or_ptr, 0);
            ir::Value tag_ptr = ir::Value::from_register(tag_ptr_reg, ir::Primitive::I32);
            context.append_store(ir::Operand::from_int_immediate(tag, ir::Primitive::I32), tag_ptr);

            const ir::Type &data_type = struct_->get_members()[1].type;
            ir::VirtualRegister data_ptr_reg = context.append_memberptr(stored_val.value_or_ptr, 1);
            ir::Value data_ptr = ir::Value::from_register(data_ptr_reg, data_type.ref());

            ExprIRBuilder data_builder(context, node);
            data_builder.set_coercion_level(coercion_level + 1);
            data_builder.build({data_ptr});

            return stored_val;
        }
    }

    switch (node->get_type()) {
        case lang::AST_INT_LITERAL: return build_int_literal(node);
        case lang::AST_FLOAT_LITERAL: return build_float_literal(node);
        case lang::AST_CHAR_LITERAL: return build_char_literal(node);
        case lang::AST_STRING_LITERAL: return build_string_literal(node);
        case lang::AST_ARRAY_EXPR: return build_array_literal(node, reqs);
        case lang::AST_MAP_EXPR: return build_map_literal(node);
        case lang::AST_TUPLE_EXPR: return build_tuple_literal(node, reqs);
        case lang::AST_CLOSURE: return ClosureIRBuilder(context, node).build(reqs);
        case lang::AST_FALSE: return create_immediate(0, ir::Primitive::I8);
        case lang::AST_TRUE: return create_immediate(1, ir::Primitive::I8);
        case lang::AST_NULL: return create_immediate(0, ir::Type(ir::Primitive::VOID, 1));
        case lang::AST_OPERATOR_EQ:
        case lang::AST_OPERATOR_NE:
        case lang::AST_OPERATOR_GT:
        case lang::AST_OPERATOR_LT:
        case lang::AST_OPERATOR_GE:
        case lang::AST_OPERATOR_LE:
        case lang::AST_OPERATOR_AND:
        case lang::AST_OPERATOR_OR: return build_bool_expr(node, reqs);
        case lang::AST_OPERATOR_ADD:
        case lang::AST_OPERATOR_SUB:
        case lang::AST_OPERATOR_MUL:
        case lang::AST_OPERATOR_DIV:
        case lang::AST_OPERATOR_MOD:
        case lang::AST_OPERATOR_BIT_AND:
        case lang::AST_OPERATOR_BIT_OR:
        case lang::AST_OPERATOR_BIT_XOR:
        case lang::AST_OPERATOR_SHL:
        case lang::AST_OPERATOR_SHR: return build_binary_operation(node, reqs);
        case lang::AST_OPERATOR_NEG: return build_neg(node, reqs);
        case lang::AST_OPERATOR_REF: return build_ref(node);
        case lang::AST_STAR_EXPR: return build_deref(node, reqs);
        case lang::AST_OPERATOR_NOT: return build_not(node, reqs);
        case lang::AST_IDENTIFIER:
        case lang::AST_DOT_OPERATOR:
        case lang::AST_SELF: return build_location(node, reqs);
        case lang::AST_FUNCTION_CALL: return build_call(node, reqs);
        case lang::AST_ARRAY_ACCESS: return build_bracket_expr(node->as<lang::BracketExpr>(), reqs);
        case lang::AST_CAST: return build_cast(node);
        case lang::AST_STRUCT_INSTANTIATION: return build_struct_literal(node, lang::STRUCT_LITERAL_VALUES, reqs);
        case lang::AST_ANON_STRUCT_LITERAL: return build_struct_literal(node, 0, reqs);
        case lang::AST_META_EXPR: return build_meta_expr(node);
        case lang::AST_META_FIELD_ACCESS:
        case lang::AST_META_METHOD_CALL: return build_node(node->as<lang::MetaExpr>()->get_value(), reqs);
        default: ASSERT_UNREACHABLE;
    }
}

StoredValue ExprIRBuilder::build_int_literal(lang::ASTNode *node) {
    const LargeInt &value = node->as<lang::IntLiteral>()->get_value();
    ir::Type type = IRBuilderUtils::build_type(lang_type);
    ir::Value immediate = ir::Value::from_int_immediate(value, type);

    return StoredValue::create_value(immediate, context);
}

StoredValue ExprIRBuilder::build_float_literal(lang::ASTNode *node) {
    double value = std::stod(node->get_value());
    ir::Type type = IRBuilderUtils::build_type(lang_type);
    ir::Value immediate = ir::Value::from_fp_immediate(value, type);

    return StoredValue::create_value(immediate, context);
}

StoredValue ExprIRBuilder::build_char_literal(lang::ASTNode *node) {
    unsigned index = 0;
    char encoded_val = encode_char(node->get_value(), index);
    ir::Value immediate = ir::Value::from_int_immediate(encoded_val, ir::Type(ir::Primitive::I8));
    return StoredValue::create_value(immediate, context);
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

StoredValue ExprIRBuilder::build_array_literal(lang::ASTNode *node, StorageReqs reqs) {
    unsigned num_elements = node->get_children().size();

    lang::Structure *lang_struct = lang_type->get_structure();
    ir::Type struct_type(lang_struct->get_ir_struct());

    lang::Function *create_func = lang_struct->get_symbol_table()->get_symbol("sized")->get_func();
    std::vector<ir::Value> args = {ir::Value::from_int_immediate(num_elements, ir::Primitive::I32)};

    StoredValue stored_val;
    if (reqs.dst) {
        stored_val = IRBuilderUtils::build_call({create_func, args}, *reqs.dst, context);
    } else {
        stored_val = IRBuilderUtils::build_call(create_func, args, context);
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

StoredValue ExprIRBuilder::build_struct_literal(lang::ASTNode *node, unsigned values_index, StorageReqs reqs) {
    lang::ASTNode *values_node = node->get_child(values_index);

    ir::Structure *struct_ = lang_type->get_structure()->get_ir_struct();
    StoredValue stored_val = StoredValue::alloc(ir::Type(struct_), reqs, context);
    ir::Value val_ptr = stored_val.value_or_ptr;

    for (lang::ASTNode *field_val_node : values_node->get_children()) {
        lang::ASTNode *field_name_node = field_val_node->get_child(0);
        lang::ASTNode *field_value_node = field_val_node->get_child(1);

        if (field_val_node->get_child(1)->get_type() == lang::AST_UNDEFINED) {
            continue;
        }

        const std::string &field_name = field_name_node->get_value();
        int field_offset = struct_->get_member_index(field_name);
        ir::VirtualRegister field_reg = context.append_memberptr(val_ptr, field_offset);

        ExprIRBuilder(context, field_value_node).build_and_store(field_reg);
    }

    return stored_val;
}

StoredValue ExprIRBuilder::build_tuple_literal(lang::ASTNode *node, StorageReqs reqs) {
    ir::Type type = IRBuilderUtils::build_type(lang_type);

    StoredValue stored_val = StoredValue::alloc(type, reqs, context);
    ir::Value val_ptr = stored_val.value_or_ptr;

    for (unsigned int i = 0; i < node->get_children().size(); i++) {
        lang::ASTNode *child = node->get_child(i);
        ir::Type member_type = type.get_tuple_types()[i];
        ir::VirtualRegister offset_ptr_reg = context.append_memberptr(val_ptr, i);
        ExprIRBuilder(context, child).build_and_store(offset_ptr_reg);
    }

    return stored_val;
}

StoredValue ExprIRBuilder::build_binary_operation(lang::ASTNode *node, StorageReqs reqs) {
    if (is_overloaded_operator_call(node)) {
        return build_overloaded_operator_call(node, reqs);
    }

    lang::Expr *lhs = node->get_child(0)->as<lang::Expr>();
    lang::Expr *rhs = node->get_child(1)->as<lang::Expr>();
    lang::ASTNodeType op = node->get_type();

    bool is_signed = lhs->get_data_type()->is_signed_int();

    ir::Value lhs_val = build_into_value(lhs);
    ir::Value rhs_val = build_into_value(rhs);

    ir::VirtualRegister reg = context.get_current_func()->next_virtual_reg();

    ir::Opcode opcode;
    bool commutative = false;

    if (lhs_val.get_type().get_ptr_depth() != 0) {
        switch (op) {
            case lang::AST_OPERATOR_ADD: opcode = ir::Opcode::OFFSETPTR; break;
            default: ASSERT_UNREACHABLE;
        }
    } else if (!lhs_val.get_type().is_floating_point()) {
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
        rhs_val.set_type(ir::Type(ir::Primitive::I8));
    }

    context.get_cur_block().append(ir::Instruction(opcode, reg, {lhs_val, rhs_val}));
    return StoredValue::create_value(ir::Value::from_register(reg, lhs_val.get_type()), context);
}

StoredValue ExprIRBuilder::build_overloaded_operator_call(lang::ASTNode *node, StorageReqs reqs) {
    lang::ASTNode *lhs = node->get_child(0);
    lang::ASTNode *rhs = node->get_child(1);
    lang::Function *func = node->as<lang::OperatorExpr>()->get_operator_func();

    ir::Value lhs_val = ExprIRBuilder(context, lhs).build_into_ptr();
    ir::Value rhs_val;

    if (func->get_type().param_types[1]->get_kind() == lang::DataType::Kind::POINTER) {
        rhs_val = ExprIRBuilder(context, rhs).build_into_ptr();
    } else {
        rhs_val = ExprIRBuilder(context, rhs).build_into_value_if_possible();
    }

    IRBuilderUtils::FuncCall call{
        .func = func,
        .args = {lhs_val, rhs_val},
    };

    ir::Value dst;
    if (reqs.dst) {
        dst = *reqs.dst;
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

StoredValue ExprIRBuilder::build_neg(lang::ASTNode *node, StorageReqs reqs) {
    ir::Value value_to_negate = build_into_value(node->get_child());
    ir::VirtualRegister dest = context.get_current_func()->next_virtual_reg();
    ir::Type type = value_to_negate.get_type();

    if (!type.is_floating_point()) {
        ir::Operand zero = ir::Operand::from_int_immediate(0, type);
        context.get_cur_block().append(ir::Instruction(ir::Opcode::SUB, dest, {zero, value_to_negate}));
    } else {
        ir::Operand zero = ir::Operand::from_fp_immediate(0.0, type);
        context.get_cur_block().append(ir::Instruction(ir::Opcode::FSUB, dest, {zero, value_to_negate}));
    }

    return StoredValue::create_value(ir::Operand::from_register(dest, type), context);
}

StoredValue ExprIRBuilder::build_ref(lang::ASTNode *node) {
    StoredValue stored_val = build_node(node->get_child(), StorageReqs::FORCE_REFERENCE);

    // Convert the pointer to a value itself to take the address.
    return StoredValue::create_value(stored_val.value_or_ptr, context);
}

StoredValue ExprIRBuilder::build_deref(lang::ASTNode *node, StorageReqs reqs) {
    lang::Expr *child_node = node->get_child()->as<lang::Expr>();

    ExprIRBuilder child_builder(context, node->get_child());
    StoredValue stored_val = child_builder.build_into_stored_val_if_possible();

    lang::DataType *child_type = child_node->get_data_type();

    if (child_type->get_kind() == lang::DataType::Kind::POINTER) {
        // Convert the value to a pointer to dereference the pointer.
        // build_and_store will then dereference the value because 'reference' is true.
        return StoredValue::create_ptr(stored_val.value_or_ptr, context);
    } else if (child_type->get_kind() == lang::DataType::Kind::STRUCT) {
        lang::Structure *struct_ = child_type->get_structure();
        lang::Function *func = struct_->get_method_table().get_function("deref");
        return IRBuilderUtils::build_call(func, {stored_val.value_or_ptr}, context);
    } else {
        assert(!"invalid deref");
        return {};
    }
}

StoredValue ExprIRBuilder::build_not(lang::ASTNode *node, StorageReqs reqs) {
    return build_bool_expr(node, reqs);
}

StoredValue ExprIRBuilder::build_location(lang::ASTNode *node, StorageReqs reqs) {
    LocationIRBuilder location_builder(context, node);
    ir::Value value = location_builder.build(true);

    if (location_builder.is_operand_ref()) {
        return StoredValue::create_ptr(value, context);
    } else {
        assert(!reqs.force_reference);
        return StoredValue::create_value(value, context);
    }
}

StoredValue ExprIRBuilder::build_call(lang::ASTNode *node, StorageReqs reqs) {
    lang::ASTNode *location_node = node->get_child(lang::CALL_LOCATION);

    const std::optional<lang::Location> &location = location_node->as<lang::Expr>()->get_location();
    if (location && location->get_last_element().is_union_case()) {
        return build_union_case_expr(node, reqs);
    } else {
        return build_func_call(node, reqs);
    }
}

StoredValue ExprIRBuilder::build_union_case_expr(lang::ASTNode *node, StorageReqs reqs) {
    lang::ASTNode *location_node = node->get_child(lang::CALL_LOCATION);
    lang::ASTNode *args_node = node->get_child(lang::CALL_ARGS);

    lang::UnionCase *lang_case = location_node->as<lang::Expr>()->get_location()->get_last_element().get_union_case();
    ir::Structure *struct_ = lang_case->get_ir_struct();

    StoredValue stored_val = StoredValue::alloc(struct_, reqs, context);
    ir::Value val_ptr = stored_val.value_or_ptr;

    for (unsigned i = 0; i < args_node->get_children().size(); i++) {
        lang::ASTNode *arg_node = args_node->get_child(i);
        if (arg_node->get_type() == lang::AST_UNDEFINED) {
            continue;
        }

        ir::VirtualRegister field_reg = context.append_memberptr(val_ptr, i);
        ExprIRBuilder(context, arg_node).build_and_store(field_reg);
    }

    return stored_val;
}

StoredValue ExprIRBuilder::build_func_call(lang::ASTNode *node, StorageReqs reqs) {
    FuncCallIRBuilder func_call_builder(context, node);
    ir::VirtualRegister dst = func_call_builder.build(reqs.dst ? *reqs.dst : StorageReqs(), true);

    ir::Type type = IRBuilderUtils::build_type(lang_type);

    if (!is_return_by_ref(type)) {
        return StoredValue::create_value(ir::Value::from_register(dst, type), context);
    } else {
        type.set_ptr_depth(type.get_ptr_depth() + 1);
        return StoredValue::create_ptr(ir::Value::from_register(dst, type), context);
    }
}

StoredValue ExprIRBuilder::build_bracket_expr(lang::BracketExpr *node, StorageReqs reqs) {
    assert(node->get_kind() == lang::BracketExpr::Kind::INDEX);

    lang::ASTNode *base_node = node->get_child(0);
    lang::ASTNode *index_node = node->get_child(1)->get_child(0);

    lang::DataType *type = node->get_child(0)->as<lang::Expr>()->get_data_type();

    if (type->get_kind() == lang::DataType::Kind::POINTER) {
        StoredValue built_pointer = ExprIRBuilder(context, base_node).build_into_stored_val_if_possible();
        StoredValue built_index = ExprIRBuilder(context, index_node).build_into_stored_val_if_possible();
        return build_offset_ptr(built_pointer, built_index);
    } else if (type->get_kind() == lang::DataType::Kind::STATIC_ARRAY) {
        StoredValue built_array = build_node(base_node, StorageReqs::FORCE_REFERENCE);
        StoredValue built_index = ExprIRBuilder(context, index_node).build_into_stored_val_if_possible();
        return build_offset_ptr(built_array, built_index);
    } else if (type->get_kind() == lang::DataType::Kind::STRUCT) {
        StoredValue built_array = ExprIRBuilder(context, base_node).build_into_stored_val_if_possible();
        StoredValue built_index = ExprIRBuilder(context, index_node).build_into_stored_val_if_possible();

        lang::Structure *struct_ = base_node->as<lang::Expr>()->get_data_type()->get_structure();
        lang::Function *ref_func = struct_->get_method_table().get_function("ref");

        ir::VirtualRegister pointer_reg = context.get_current_func()->next_virtual_reg();
        ir::Type type = IRBuilderUtils::build_type(lang_type).ref();
        ir::Value pointer = ir::Operand::from_register(pointer_reg, type);

        context.get_cur_block().append(ir::Instruction(
            ir::Opcode::CALL,
            pointer_reg,
            {ir::Operand::from_func(ref_func->get_ir_func(), type), built_array.value_or_ptr, built_index.value_or_ptr}
        ));

        return StoredValue::create_ptr(pointer, context);
    } else {
        return {{}, false};
    }
}

StoredValue ExprIRBuilder::build_cast(lang::ASTNode *node) {
    lang::DataType *old_lang_type = node->get_child(0)->as<lang::Expr>()->get_data_type();
    lang::DataType *new_lang_type = node->as<lang::Expr>()->get_data_type();

    ir::Value old_val = ExprIRBuilder(context, node->get_child(0)).build_into_value();
    ir::Value new_val = Conversion::build(context, old_val, old_lang_type, new_lang_type);
    return StoredValue::create_value(new_val, context);
}

StoredValue ExprIRBuilder::build_meta_expr(lang::ASTNode *node) {
    lang::ASTNode *args_node = node->get_child(lang::META_EXPR_ARGS);
    lang::ASTNode *type_node = args_node->get_child(0);

    ir::Type type = IRBuilderUtils::build_type(type_node->as<lang::Expr>()->get_data_type());
    int size = get_size(type);

    ir::Value immediate = ir::Value::from_int_immediate(size, ir::Type(ir::Primitive::I64));
    return StoredValue::create_value(immediate, context);
}

StoredValue ExprIRBuilder::build_bool_expr(lang::ASTNode *node, StorageReqs reqs) {
    if (is_overloaded_operator_call(node)) {
        return build_overloaded_operator_call(node, reqs);
    }

    int label_id = context.next_cmp_to_val_id();

    ir::BasicBlockIter true_block = context.create_block("cmp." + std::to_string(label_id) + ".true");
    ir::BasicBlockIter false_block = context.create_block("cmp." + std::to_string(label_id) + ".false");
    ir::BasicBlockIter end_block = context.create_block("cmp." + std::to_string(label_id) + ".end");

    ir::VirtualRegister reg = context.get_current_func()->next_virtual_reg();
    context.append_alloca(reg, ir::Type(ir::Primitive::I8));

    BoolExprIRBuilder(context, node, true_block, false_block).build();

    ir::Operand dst = ir::Operand::from_register(reg, ir::Type(ir::Primitive::I8, 1));
    context.append_block(true_block);
    context.append_store(ir::Operand::from_int_immediate(1, ir::Type(ir::Primitive::I8)), dst);
    context.append_jmp(end_block);
    context.append_block(false_block);
    context.append_store(ir::Operand::from_int_immediate(0, ir::Type(ir::Primitive::I8)), dst);
    context.append_jmp(end_block);
    context.append_block(end_block);

    return StoredValue::create_ptr(dst, context);
}

StoredValue ExprIRBuilder::build_implicit_optional(lang::Expr *expr, StorageReqs reqs) {
    lang::Structure *lang_struct = lang_type->get_structure();
    ir::Type struct_type(lang_struct->get_ir_struct());

    if (((lang::ASTNode *)expr)->get_type() != lang::AST_NONE) {
        ExprIRBuilder val_builder(context, expr);
        val_builder.set_coercion_level(coercion_level + 1);
        ir::Value val = val_builder.build_into_value_if_possible();

        lang::Function *some_func = lang_struct->get_symbol_table()->get_function("new_some");
        return IRBuilderUtils::build_call(some_func, {val}, context);
    } else {
        lang::Function *none_func = lang_struct->get_symbol_table()->get_function("new_none");
        return IRBuilderUtils::build_call(none_func, {}, context);
    }
}

StoredValue ExprIRBuilder::build_implicit_result(lang::Expr *expr, StorageReqs reqs) {
    lang::Structure *lang_struct = lang_type->get_structure();
    ir::Type struct_type(lang_struct->get_ir_struct());

    lang::DataType *error_lang_type = lang::StandardTypes::get_result_error_type(lang_type);
    lang::DataType *coercion_from = expr->get_coercion_chain()[coercion_level + 1];
    bool is_error = coercion_from->equals(error_lang_type);

    const std::string &func_name = is_error ? "failure" : "success";
    lang::Function *func = lang_struct->get_symbol_table()->get_function(func_name);

    ExprIRBuilder val_builder(context, expr);
    val_builder.set_coercion_level(coercion_level + 1);
    ir::Value val = val_builder.build_into_value_if_possible();

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
    context.get_current_mod()->add(
        ir::Global(name, ir::Type(ir::Primitive::I8, 1), ir::Operand::from_string(encoded_val))
    );
    return StoredValue::create_value(ir::Operand::from_global(name, ir::Type(ir::Primitive::I8, 1)), context);
}

StoredValue ExprIRBuilder::build_offset_ptr(StoredValue pointer, StoredValue index) {
    ir::Type type = pointer.value_or_ptr.get_type().deref();
    ir::Operand offset = {pointer.value_or_ptr};

    if (index.value_or_ptr.is_immediate()) {
        offset = ir::Operand::from_int_immediate(index.value_or_ptr.get_int_immediate(), type);
    } else if (index.value_or_ptr.is_register()) {
        ir::Type required_type = context.get_target()->get_data_layout().get_usize_type();
        if (index.value_or_ptr.get_type() == required_type || true) {
            offset = ir::Operand::from_register(index.value_or_ptr.get_register(), required_type);
        } else {
            // TODO: usize smaller than size
            ir::VirtualRegister temp_reg = context.get_current_func()->next_virtual_reg();
            context.get_cur_block().append(ir::Instruction(
                ir::Opcode::SEXTEND,
                temp_reg,
                {ir::Operand::from_register(index.value_or_ptr.get_register(), index.value_or_ptr.get_type()),
                 ir::Operand::from_type(required_type)}
            ));
            offset = ir::Operand::from_register(temp_reg, required_type);
        }
    }

    ir::VirtualRegister offset_ptr = context.append_offsetptr(pointer.value_or_ptr, offset);
    return StoredValue::create_ptr(ir::Value::from_register(offset_ptr, pointer.value_or_ptr.get_type()), context);
}

StoredValue ExprIRBuilder::create_immediate(LargeInt value, ir::Type type) {
    return StoredValue::create_value(ir::Value::from_int_immediate(value, type), context);
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
