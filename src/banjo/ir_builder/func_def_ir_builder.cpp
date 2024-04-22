#include "func_def_ir_builder.hpp"

#include "ast/ast_block.hpp"
#include "ast/ast_child_indices.hpp"
#include "ast/decl.hpp"
#include "ir_builder/block_ir_builder.hpp"
#include "ir_builder/ir_builder_utils.hpp"
#include "ir_builder/name_mangling.hpp"
#include "symbol/symbol_table.hpp"

#include <utility>

namespace ir_builder {

void FuncDefIRBuilder::build() {
    lang::ASTNode *block_node = node->get_child(lang::FUNC_BLOCK);
    lang::Function *lang_func = node->as<lang::ASTFunc>()->get_symbol();
    std::string name = IRBuilderUtils::get_func_link_name(lang_func);
    build(lang_func, block_node, name);
}

void FuncDefIRBuilder::build_closure(lang::Function *lang_func) {
    lang::ASTNode *block_node = node->get_child(lang::CLOSURE_BLOCK);
    std::string name = IRBuilderUtils::get_func_link_name(lang_func);
    build(lang_func, block_node, name);
}

void FuncDefIRBuilder::build(lang::Function *lang_func, lang::ASTNode *block_node, std::string name) {
    context.set_current_lang_func(lang_func);

    create_func(lang_func, std::move(name));
    ir::Function *func = lang_func->get_ir_func();
    context.set_current_func(func);
    context.set_cur_block_iter(context.get_current_func()->get_entry_block_iter());

    std::vector<ir::VirtualRegister> arg_regs;
    for (unsigned int i = 0; i < func->get_params().size(); i++) {
        arg_regs.push_back(func->next_virtual_reg());
    }
    context.set_current_arg_regs(arg_regs);

    lang::SymbolTable *symbol_table = static_cast<lang::ASTBlock *>(block_node)->get_symbol_table();
    init_lang_params(lang_func, symbol_table->get_parameters());

    if (has_return_value()) {
        ir::VirtualRegister current_return_reg = func->next_virtual_reg();
        context.set_current_return_reg(current_return_reg);
        context.append_alloca(current_return_reg, func->get_return_type());
    }

    build_arg_store(func);
    build_block(block_node);
    build_return();

    func->set_global(is_external(lang_func));
    if (lang_func->is_dllexport() && context.get_target()->get_descr().is_windows()) {
        context.get_current_mod()->add_dll_export(func->get_name());
    }

    context.set_current_arg_regs({});
    context.get_current_mod()->add(func);
    context.set_current_func(nullptr);
}

void FuncDefIRBuilder::create_func(lang::Function *lang_func, std::string name) {
    std::vector<ir::Type> param_list = IRBuilderUtils::build_params(lang_func->get_type().param_types, context);
    ir::Type return_type = IRBuilderUtils::build_type(lang_func->get_type().return_type);

    if (lang_func->is_return_by_ref()) {
        param_list.insert(param_list.begin(), return_type.ref());
        return_type = ir::Type(ir::Primitive::VOID);
    }

    ir::Function *ir_func = lang_func->get_ir_func();
    ir_func->set_name(std::move(name));
    ir_func->set_params(param_list);
    ir_func->set_return_type(return_type);
    ir_func->set_calling_conv(context.get_target()->get_default_calling_conv());

    for (unsigned int i = 0; i < param_list.size(); i++) {
        ir_func->get_param_regs().push_back(ir_func->next_virtual_reg());
    }
}

void FuncDefIRBuilder::init_lang_params(lang::Function *lang_func, std::vector<lang::Parameter *> &lang_params) {
    int lang_arg_start_index = lang_func->is_return_by_ref() ? 1 : 0;

    if (context.get_current_closure()) {
        lang_arg_start_index += 1;
    }

    for (unsigned int i = 0; i < lang_params.size(); i++) {
        lang::Parameter *lang_param = lang_params[i];

        ir::Type type = IRBuilderUtils::build_type(lang_param->get_data_type());
        bool pass_by_ref = is_pass_by_ref(type);
        lang_param->set_pass_by_ref(pass_by_ref);
        lang_param->set_virtual_reg(context.get_current_arg_regs()[i + lang_arg_start_index]);
    }
}

void FuncDefIRBuilder::build_arg_store(ir::Function *func) {
    for (unsigned i = 0; i < func->get_params().size(); i++) {
        ir::Type param_type = func->get_params()[i];
        ir::VirtualRegister value_reg = func->next_virtual_reg();
        ir::VirtualRegister store_reg = context.get_current_arg_regs()[i];

        ir::Instruction &alloca_instr = context.append_alloca(store_reg, param_type);
        alloca_instr.set_flag(ir::Instruction::FLAG_ARG_STORE);

        ir::Instruction &loadarg_instr =
            context.append_loadarg(value_reg, ir::Operand::from_int_immediate(i, param_type.ref()));
        loadarg_instr.set_flag(ir::Instruction::FLAG_SAVE_ARG);

        ir::Instruction &store_instr = context.append_store(
            ir::Operand::from_register(value_reg, param_type),
            ir::Operand::from_register(store_reg, param_type.ref())
        );
        store_instr.set_flag(ir::Instruction::FLAG_SAVE_ARG);
    }
}

void FuncDefIRBuilder::build_block(lang::ASTNode *block) {
    BlockIRBuilder block_builder(context, block);
    context.set_cur_func_exit(block_builder.create_exit());
    block_builder.build();
}

void FuncDefIRBuilder::build_return() {
    if (has_return_value()) {
        ir::VirtualRegister val_reg = context.get_current_func()->next_virtual_reg();
        ir::VirtualRegister return_reg = context.get_current_return_reg();
        ir::Type return_reg_type = context.get_current_func()->get_return_type().ref();
        context.append_load(val_reg, ir::Operand::from_register(return_reg, return_reg_type));
        context.append_ret(ir::Operand::from_register(val_reg, return_reg_type.deref()));
    } else if (context.get_current_lang_func()->get_name() == "main") {
        context.append_ret(ir::Operand::from_int_immediate(0, ir::Type(ir::Primitive::I32)));
    } else {
        context.append_ret();
    }
}

bool FuncDefIRBuilder::has_return_value() {
    lang::Function *lang_func = context.get_current_lang_func();
    bool is_returning_void = lang_func->get_type().return_type->is_primitive_of_type(lang::PrimitiveType::VOID);
    bool is_return_by_ref = lang_func->is_return_by_ref();
    return !is_returning_void && !is_return_by_ref;
}

bool FuncDefIRBuilder::is_external(lang::Function *func) {
    return func->is_exposed() || func->is_dllexport() || func->get_name() == "main";
}

} // namespace ir_builder
