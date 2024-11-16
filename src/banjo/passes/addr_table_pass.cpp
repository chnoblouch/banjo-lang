#include "addr_table_pass.hpp"

#include "banjo/ir/addr_table.hpp"

namespace banjo {

namespace passes {

AddrTablePass::AddrTablePass(target::Target *target) : Pass("addr-table", target) {}

void AddrTablePass::run(ir::Module &mod) {
    if (!mod.get_addr_table()) {
        ir::AddrTable addr_table;

        for (const ir::Function *func : mod.get_functions()) {
            addr_table.append(func->get_name());
        }

        for (const ir::FunctionDecl &external_func : mod.get_external_functions()) {
            addr_table.append(external_func.get_name());
        }

        for (const ir::GlobalDecl &external_global : mod.get_external_globals()) {
            addr_table.append(external_global.get_name());
        }

        mod.set_addr_table(addr_table);
    } else {
        mod.add(ir::GlobalDecl("addr_table", ir::Primitive::I64));
    }

    for (ir::Function *func : mod.get_functions()) {
        replace_uses(mod, func);
    }
}

void AddrTablePass::replace_uses(ir::Module &mod, ir::Function *func) {
    for (ir::BasicBlock &basic_block : func->get_basic_blocks()) {
        replace_uses(mod, func, basic_block);
    }
}

void AddrTablePass::replace_uses(ir::Module &mod, ir::Function *func, ir::BasicBlock &basic_block) {
    ir::AddrTable &addr_table = *mod.get_addr_table();

    for (ir::InstrIter iter = basic_block.begin(); iter != basic_block.end(); ++iter) {
        for (unsigned i = 0; i < iter->get_operands().size(); i++) {
            ir::Operand &operand = iter->get_operand(i);
            if (!operand.is_symbol()) {
                continue;
            }

            std::optional<unsigned> index = addr_table.find_index(operand.get_symbol_name());
            if (!index) {
                continue;
            }

            unsigned offset = addr_table.compute_offset(*index);

            ir::VirtualRegister ptr_reg = func->next_virtual_reg();
            ir::VirtualRegister reg = func->next_virtual_reg();

            ir::Operand ssa_base = ir::Operand::from_global("addr_table", ir::Primitive::ADDR);
            ir::Operand ssa_offset = ir::Operand::from_int_immediate(offset, ir::Primitive::I64);
            basic_block.insert_before(iter, ir::Instruction(ir::Opcode::ADD, ptr_reg, {ssa_base, ssa_offset}));

            ir::Operand ssa_type = ir::Operand::from_type(ir::Primitive::ADDR);
            ir::Operand ssa_ptr = ir::Operand::from_register(ptr_reg, ir::Primitive::ADDR);
            basic_block.insert_before(iter, ir::Instruction(ir::Opcode::LOAD, reg, {ssa_type, ssa_ptr}));

            operand = ir::Operand::from_register(reg, operand.get_type());
        }
    }
}

} // namespace passes

} // namespace banjo
