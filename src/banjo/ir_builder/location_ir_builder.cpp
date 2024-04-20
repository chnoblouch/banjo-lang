#include "location_ir_builder.hpp"

#include "ast/expr.hpp"
#include "ir_builder/expr_ir_builder.hpp"
#include "ir_builder/ir_builder_utils.hpp"
#include "symbol/constant.hpp"
#include "symbol/data_type.hpp"
#include "symbol/enumeration.hpp"
#include "symbol/global_variable.hpp"
#include "symbol/local_variable.hpp"
#include "symbol/location.hpp"
#include "symbol/parameter.hpp"

namespace ir_builder {

ir::Value LocationIRBuilder::build(bool return_value) {
    const lang::Location &location = *node->as<lang::Expr>()->get_location();
    return build_location(location, return_value);
}

ir::Value LocationIRBuilder::build_location(const lang::Location &location, bool return_value) {
    dest = context.get_current_func()->next_virtual_reg();
    build(location);

    // TODO: replace this
    if (return_value && func && !func->is_native() &&
        location.get_type()->get_kind() == lang::DataType::Kind::FUNCTION) {
        operand = ir::Operand::from_func(func->get_ir_func(), ir::Type(ir::Primitive::VOID, 1));
    }

    return operand;
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
        ir::Type ir_type = ir::Type(ir::Primitive::VOID, 1);

        if (!element.get_func()->is_native()) {
            operand = ir::Operand::from_func(element.get_func()->get_ir_func(), ir_type);
        } else {
            std::string link_name = IRBuilderUtils::get_func_link_name(element.get_func());
            operand = ir::Operand::from_extern_func(link_name, ir_type);
        }

        operand_ref = false;
    } else if (element.is_self()) {
        int self_param_index = context.get_current_lang_func()->is_return_by_ref() ? 1 : 0;

        ir::VirtualRegister self_reg = context.get_current_arg_regs()[self_param_index];
        ir::Type self_type = context.get_current_func()->get_params()[self_param_index].ref();

        operand = ir::Operand::from_register(self_reg, self_type);
        operand_ref = true;
    } else if (element.is_enum_variant()) {
        const LargeInt &value = element.get_enum_variant()->get_value();
        operand = ir::Operand::from_int_immediate(value, ir::Type(ir::Primitive::I32));
        operand_ref = false;
    } else if (element.is_expr()) {
        ExprIRBuilder expr_builder(context, element.get_expr());
        operand = expr_builder.build_into_ptr();
        operand_ref = true;
    } else {
        assert(!"invalid root location element");
    }
}

void LocationIRBuilder::build_var(lang::Variable *var) {
    this->var = var;

    if (is_captured_var(var)) {
        build_captured_var(var);
    } else {
        operand = var->as_ir_operand(context);
        operand_ref = var->is_ir_operand_reference();
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

    int member_index = 0;
    auto it = std::find(captured_vars.begin(), captured_vars.end(), var);
    if (it == captured_vars.end()) {
        captured_vars.push_back(var);
        member_index = captured_vars.size() - 1;
    } else {
        member_index = it - captured_vars.begin();
    }

    ir::VirtualRegister arg_ptr_reg = context.get_current_arg_regs()[0];
    ir::Operand arg_ptr = ir::Operand::from_register(arg_ptr_reg, context_type.ref().ref());
    ir::VirtualRegister context_ptr_reg = context.append_load(arg_ptr);
    ir::Operand context_ptr = ir::Operand::from_register(context_ptr_reg, context_type.ref());
    ir::VirtualRegister member_ptr_reg = context.append_memberptr(context_ptr, member_index);
    ir::Type member_type = IRBuilderUtils::build_type(var->get_data_type());
    operand = ir::Operand::from_register(member_ptr_reg, member_type.ref());
    operand_ref = true;
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
        ir::VirtualRegister reg = context.append_memberptr(operand, element.get_tuple_index());
        ir::Type type = IRBuilderUtils::build_type(element.get_type()).ref();
        operand = ir::Operand::from_register(reg, type);
        operand_ref = true;
    } else {
        assert(false && "invalid non-root location element");
    }
}

void LocationIRBuilder::build_struct_field_access(lang::StructField *field, lang::Structure *struct_) {
    int offset = struct_->get_field_index(field);
    ir::Type type = IRBuilderUtils::build_type(field->get_type()).ref();

    context.append_memberptr(dest, operand, offset);
    operand = ir::Operand::from_register(dest, type);
    dest = context.get_current_func()->next_virtual_reg();
}

void LocationIRBuilder::build_union_case_field_access(lang::UnionCaseField *field, lang::UnionCase *case_) {
    unsigned index = case_->get_field_index(field);
    ir::Type type = IRBuilderUtils::build_type(field->get_type()).ref();

    context.append_memberptr(dest, operand, index);
    operand = ir::Operand::from_register(dest, type);
    dest = context.get_current_func()->next_virtual_reg();
}

void LocationIRBuilder::build_direct_method_call(lang::Function *method) {
    self_operand = operand;
    self_ptr = false;
    func = method;
}

void LocationIRBuilder::build_ptr_field_access(lang::StructField *field, lang::Structure *struct_) {
    ir::Type base_type;
    ir::Type type = IRBuilderUtils::build_type(field->get_type()).ref();

    if (operand_ref) {
        context.append_load(dest, operand);
        base_type = operand.get_type().deref();
    } else {
        dest = operand.get_register();
        base_type = operand.get_type();
    }

    ir::Operand base = ir::Operand::from_register(dest, base_type);
    int field_index = struct_->get_field_index(field);
    ir::VirtualRegister offset_ptr_reg = context.append_memberptr(base, field_index);

    operand = ir::Operand::from_register(offset_ptr_reg, type);
    operand_ref = true;

    dest = context.get_current_func()->next_virtual_reg();
}

void LocationIRBuilder::build_ptr_method_call(lang::Function *method) {
    self_operand = operand;
    self_ptr = operand_ref;
    func = method;
}

} // namespace ir_builder
