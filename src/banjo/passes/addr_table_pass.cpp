#include "addr_table_pass.hpp"

#include "banjo/ssa/addr_table.hpp"

namespace banjo {

namespace passes {

AddrTablePass::AddrTablePass(target::Target *target) : Pass("addr-table", target) {}

void AddrTablePass::run(ssa::Module &mod) {
    if (!mod.get_addr_table()) {
        ssa::AddrTable addr_table;

        for (const ssa::Function *func : mod.get_functions()) {
            addr_table.append(func->get_name());
        }

        for (const ssa::FunctionDecl *external_func : mod.get_external_functions()) {
            addr_table.append(external_func->get_name());
        }

        for (const ssa::GlobalDecl *external_global : mod.get_external_globals()) {
            addr_table.append(external_global->name);
        }

        mod.set_addr_table(addr_table);
    } else {
        mod.add(new ssa::GlobalDecl{
            .name = "addr_table",
            .type = ssa::Primitive::I64,
        });
    }

    for (ssa::Function *func : mod.get_functions()) {
        replace_uses(mod, func);
    }
}

void AddrTablePass::replace_uses(ssa::Module &mod, ssa::Function *func) {
    for (ssa::BasicBlock &basic_block : func->get_basic_blocks()) {
        replace_uses(mod, func, basic_block);
    }
}

void AddrTablePass::replace_uses(ssa::Module &mod, ssa::Function *func, ssa::BasicBlock &basic_block) {
    ssa::AddrTable &addr_table = *mod.get_addr_table();

    for (ssa::InstrIter iter = basic_block.begin(); iter != basic_block.end(); ++iter) {
        for (unsigned i = 0; i < iter->get_operands().size(); i++) {
            ssa::Operand &operand = iter->get_operand(i);
            if (!operand.is_symbol()) {
                continue;
            }

            std::optional<unsigned> index = addr_table.find_index(operand.get_symbol_name());
            if (!index) {
                continue;
            }

            unsigned offset = addr_table.compute_offset(*index);

            ssa::VirtualRegister ptr_reg = func->next_virtual_reg();
            ssa::VirtualRegister reg = func->next_virtual_reg();

            ssa::GlobalDecl &ssa_global = ssa::AddrTable::DUMMY_GLOBAL;
            ssa::Operand ssa_base = ssa::Operand::from_extern_global(&ssa_global, ssa::Primitive::ADDR);
            ssa::Operand ssa_offset = ssa::Operand::from_int_immediate(offset, ssa::Primitive::I64);
            basic_block.insert_before(iter, ssa::Instruction(ssa::Opcode::ADD, ptr_reg, {ssa_base, ssa_offset}));

            ssa::Operand ssa_type = ssa::Operand::from_type(ssa::Primitive::ADDR);
            ssa::Operand ssa_ptr = ssa::Operand::from_register(ptr_reg, ssa::Primitive::ADDR);
            basic_block.insert_before(iter, ssa::Instruction(ssa::Opcode::LOAD, reg, {ssa_type, ssa_ptr}));

            operand = ssa::Operand::from_register(reg, operand.get_type());
        }
    }
}

} // namespace passes

} // namespace banjo
