#include "deinit_ir_builder.hpp"

#include "ast/ast_block.hpp"
#include "ir_builder/ir_builder_utils.hpp"
#include "ir_builder/location_ir_builder.hpp"
#include "symbol/magic_functions.hpp"
#include "symbol/structure.hpp"

#include <cassert>

namespace banjo {

namespace ir_builder {

const ir::Type FLAG_TYPE(ir::Primitive::I8);

void DeinitIRBuilder::build() {
    for (lang::DeinitInfo *info : node->as<lang::ASTBlock>()->get_deinit_values()) {
        ir::VirtualRegister flag_reg = context.append_alloca(FLAG_TYPE);

        context.append_store(
            ir::Operand::from_int_immediate(1, FLAG_TYPE),
            ir::Operand::from_register(flag_reg, ir::Primitive::ADDR)
        );

        info->flag_reg = flag_reg;
    }

    for (const lang::ValueMove &use : node->as<lang::ASTBlock>()->get_value_moves()) {
        assert(use.deinit_info->flag_reg != -1);

        context.append_store(
            ir::Operand::from_int_immediate(0, FLAG_TYPE),
            ir::Operand::from_register(use.deinit_info->flag_reg, ir::Primitive::ADDR)
        );
    }
}

void DeinitIRBuilder::build_exit() {
    for (lang::DeinitInfo *info : node->as<lang::ASTBlock>()->get_deinit_values()) {
        build_cond_deinit_call(info);
    }
}

void DeinitIRBuilder::build_cond_deinit_call(lang::DeinitInfo *info) {
    if (info->unmanaged) {
        return;
    }

    int deinit_flag_id = context.next_deinit_flag_id();
    ir::BasicBlockIter do_deinit = context.create_block("deinit.do." + std::to_string(deinit_flag_id));
    ir::BasicBlockIter skip_deinit = context.create_block("deinit.skip." + std::to_string(deinit_flag_id));

    ir::VirtualRegister loaded_flag_reg = context.get_current_func()->next_virtual_reg();
    ir::Operand flag_ptr = ir::Operand::from_register(info->flag_reg, ir::Primitive::ADDR);
    context.append_load(loaded_flag_reg, FLAG_TYPE, flag_ptr);

    context.append_cjmp(
        ir::Operand::from_register(loaded_flag_reg, FLAG_TYPE),
        ir::Comparison::EQ,
        ir::Operand::from_int_immediate(1, FLAG_TYPE),
        do_deinit,
        skip_deinit
    );

    context.append_block(do_deinit);
    build_deinit_call(info);
    context.append_jmp(skip_deinit);
    context.append_block(skip_deinit);
}

void DeinitIRBuilder::build_deinit_call(lang::DeinitInfo *info) {
    LocationIRBuilder location_builder(context, nullptr);
    ir::Value operand = location_builder.build_location(info->location, false).value_or_ptr;
    lang::Structure *struct_ = info->location.get_type()->get_structure();
    lang::Function *func = struct_->get_method_table().get_function(lang::MagicFunctions::DEINIT);
    IRBuilderUtils::build_call(func, {operand}, context);
}

} // namespace ir_builder

} // namespace banjo
