#include "location_ir_builder.hpp"

#include "banjo/ast/expr.hpp"
#include "banjo/ir_builder/expr_ir_builder.hpp"
#include "banjo/ir_builder/ir_builder_utils.hpp"
#include "banjo/ir_builder/storage.hpp"
#include "banjo/symbol/constant.hpp"
#include "banjo/symbol/data_type.hpp"
#include "banjo/symbol/enumeration.hpp"
#include "banjo/symbol/location.hpp"
#include "banjo/symbol/protocol.hpp"
#include "banjo/utils/macros.hpp"

#include <cassert>

namespace banjo {

namespace ir_builder {

StoredValue LocationIRBuilder::build(bool return_value) {
    const lang::Location &location = *node->as<lang::Expr>()->get_location();
    return build_location(location, return_value);
}

StoredValue LocationIRBuilder::build_location(const lang::Location &location, bool return_value) {
    dst = context.get_current_func()->next_virtual_reg();
    build(location);

    // TODO: replace this
    if (return_value && func && !func->is_native() &&
        location.get_type()->get_kind() == lang::DataType::Kind::FUNCTION) {
        ir::Value value = ir::Operand::from_func(func->get_ir_func(), ir::Primitive::ADDR);
        this->value = StoredValue::create_value(value);
    }

    return value;
}

void LocationIRBuilder::build(const lang::Location &location) {
    assert(location.has_root() && "location has no root");

    build_root(location.get_elements()[0]);

    if (location.get_elements().size() > 1) {
        for (int i = 1; i < location.get_elements().size(); i++) {
            lang::DataType *previous_type = location.get_elements()[i - 1].get_type();
            build_element(location.get_elements()[i], previous_type);
        }
    }
}

void LocationIRBuilder::build_root(const lang::LocationElement &element) {
    if (element.is_local()) {
        build_var(element.get_local());
    } else if (element.is_param()) {
        build_var(element.get_param());
    } else if (element.is_global()) {
        build_var(element.get_global());
    } else if (element.is_const()) {
        build_var(element.get_const());
    } else if (element.is_func()) {
        func = element.get_func();

        ir::Type type = ir::Primitive::ADDR;
        ir::Value value;

        if (!element.get_func()->is_native()) {
            value = ir::Operand::from_func(element.get_func()->get_ir_func(), type);
        } else {
            std::string link_name = IRBuilderUtils::get_func_link_name(element.get_func());
            value = ir::Operand::from_extern_func(link_name, type);
        }

        this->value = StoredValue::create_value(value);
    } else if (element.is_self()) {
        unsigned self_param_index = context.get_current_lang_func()->is_return_by_ref() ? 1 : 0;
        ir::VirtualRegister self_reg = context.get_current_arg_regs()[self_param_index];
        ir::Type self_type = context.get_current_func()->get_params()[self_param_index];
        value = StoredValue::create_reference(self_reg, self_type);
    } else if (element.is_enum_variant()) {
        const LargeInt &int_value = element.get_enum_variant()->get_value();
        ir::Type type = ir::Primitive::I32;
        ir::Value value = ir::Operand::from_int_immediate(int_value, type);
        this->value = StoredValue::create_value(value);
    } else if (element.is_expr()) {
        ExprIRBuilder expr_builder(context, element.get_expr());
        value = expr_builder.build_into_ptr();
        assert(value.reference);
    } else {
        ASSERT_UNREACHABLE;
    }
}

void LocationIRBuilder::build_var(lang::Variable *var) {
    this->var = var;

    if (is_captured_var(var)) {
        build_captured_var(var);
    } else {
        value = var->as_ir_value(context);
    }
}

bool LocationIRBuilder::is_captured_var(lang::Variable *var) {
    if (!context.get_current_closure()) {
        return false;
    }

    lang::ASTNode *closure_node = context.get_current_closure()->node;
    lang::ASTNode *var_node = var->get_node();
    return !closure_node->is_ancestor_of(var_node);
}

void LocationIRBuilder::build_captured_var(lang::Variable *var) {
    std::vector<lang::Variable *> &captured_vars = context.get_current_closure()->captured_vars;
    ir::Type context_type = context.get_current_closure()->context_type;

    unsigned member_index = 0;
    auto it = std::find(captured_vars.begin(), captured_vars.end(), var);
    if (it == captured_vars.end()) {
        captured_vars.push_back(var);
        member_index = captured_vars.size() - 1;
    } else {
        member_index = it - captured_vars.begin();
    }

    ir::VirtualRegister arg_ptr_reg = context.get_current_arg_regs()[0];
    ir::Operand arg_ptr = ir::Operand::from_register(arg_ptr_reg, ir::Primitive::ADDR);
    ir::Value context_ptr = context.append_load(ir::Primitive::ADDR, arg_ptr);
    ir::VirtualRegister member_ptr_reg = context.append_memberptr(context_type, context_ptr, member_index);
    ir::Type member_type = context.build_type(var->get_data_type());
    value = StoredValue::create_reference(member_ptr_reg, member_type);
}

void LocationIRBuilder::build_element(const lang::LocationElement &element, lang::DataType *previous_type) {
    lang::DataType::Kind prev_kind = previous_type->get_kind();

    if (element.is_field()) {
        if (prev_kind == lang::DataType::Kind::STRUCT) {
            lang::Structure *struct_ = previous_type->get_structure();
            build_struct_field_access(element.get_field(), struct_);
        } else if (prev_kind == lang::DataType::Kind::POINTER) {
            lang::Structure *struct_ = previous_type->get_base_data_type()->get_structure();
            build_ptr_field_access(element.get_field(), struct_);
        }
    } else if (element.is_union_case_field()) {
        lang::UnionCase *union_case = previous_type->get_union_case();
        build_union_case_field_access(element.get_union_case_field(), union_case);
    } else if (element.is_func()) {
        if (prev_kind == lang::DataType::Kind::STRUCT || prev_kind == lang::DataType::Kind::UNION ||
            prev_kind == lang::DataType::Kind::UNION_CASE) {
            build_direct_method_call(element.get_func());
        } else if (prev_kind == lang::DataType::Kind::POINTER) {
            build_ptr_method_call(element.get_func());
        }
    } else if (element.is_tuple_index()) {
        unsigned index = element.get_tuple_index();
        ir::VirtualRegister reg = context.append_memberptr(value.value_type, value.get_ptr(), index);
        ir::Type type = context.build_type(element.get_type());
        value = StoredValue::create_reference(reg, type);
    } else if (element.is_proto_method()) {
        build_proto_method_access(element.get_proto_method(), previous_type->get_base_data_type()->get_protocol());
    } else {
        ASSERT_UNREACHABLE;
    }
}

void LocationIRBuilder::build_struct_field_access(lang::StructField *field, lang::Structure *struct_) {
    unsigned offset = struct_->get_field_index(field);
    context.append_memberptr(dst, value.value_type, value.get_ptr(), offset);

    ir::Type type = context.build_type(field->get_type());
    value = StoredValue::create_reference(dst, type);

    dst = context.get_current_func()->next_virtual_reg();
}

void LocationIRBuilder::build_union_case_field_access(lang::UnionCaseField *field, lang::UnionCase *case_) {
    unsigned index = case_->get_field_index(field);
    context.append_memberptr(dst, value.value_type, value.get_ptr(), index);

    ir::Type type = context.build_type(field->get_type());
    value = StoredValue::create_reference(dst, type);

    dst = context.get_current_func()->next_virtual_reg();
}

void LocationIRBuilder::build_direct_method_call(lang::Function *method) {
    self = StoredValue::create_value(value.get_ptr());
    func = method;
}

void LocationIRBuilder::build_ptr_field_access(lang::StructField *field, lang::Structure *struct_) {
    if (value.reference) {
        context.append_load(dst, value.value_type, value.get_ptr());
    } else {
        dst = value.value_or_ptr.get_register();
    }

    ir::Type base_type(struct_->get_ir_struct());
    ir::Operand base = ir::Operand::from_register(dst, ir::Primitive::ADDR);
    unsigned field_index = struct_->get_field_index(field);
    ir::VirtualRegister member_ptr_reg = context.append_memberptr(base_type, base, field_index);

    ir::Type type = context.build_type(field->get_type());
    value = StoredValue::create_reference(member_ptr_reg, type);

    dst = context.get_current_func()->next_virtual_reg();
}

void LocationIRBuilder::build_ptr_method_call(lang::Function *method) {
    self = value;
    func = method;
}

void LocationIRBuilder::build_proto_method_access(lang::FunctionSignature *proto_method, lang::Protocol *proto) {
    ir::Operand base = ir::Operand::from_register(value.get_ptr().get_register(), ir::Primitive::ADDR);
    
    ir::VirtualRegister self_ptr_reg = context.append_memberptr(value.value_type, base, 0);
    ir::Value self_ptr = ir::Value::from_register(self_ptr_reg, ir::Primitive::ADDR);
    ir::Value self_val = context.append_load(ir::Primitive::ADDR, self_ptr);
    self = StoredValue::create_value(self_val);

    ir::VirtualRegister vtable_ptr_ptr_reg = context.append_memberptr(value.value_type, base, 1);
    ir::Value vtable_ptr_ptr = ir::Value::from_register(vtable_ptr_ptr_reg, ir::Primitive::ADDR);
    ir::Value vtable_ptr = context.append_load(ir::Primitive::ADDR, vtable_ptr_ptr);
    ir::Type vtable_type(proto->get_ir_vtable_struct());
    ir::VirtualRegister method_ptr_ptr_reg = context.append_memberptr(vtable_type, vtable_ptr, proto_method->index);
    ir::Value method_ptr_ptr = ir::Value::from_register(method_ptr_ptr_reg, ir::Primitive::ADDR);
    value = StoredValue::create_reference(method_ptr_ptr, ir::Primitive::ADDR);

    dst = context.get_current_func()->next_virtual_reg();
}

} // namespace ir_builder

} // namespace banjo
