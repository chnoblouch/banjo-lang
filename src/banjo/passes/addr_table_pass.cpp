#include "addr_table_pass.hpp"

namespace passes {

AddrTablePass::AddrTablePass(target::Target *target, std::optional<AddrTable> addr_table /* = {} */)
  : Pass("addr-table", target),
    addr_table(addr_table) {}

void AddrTablePass::run(ir::Module &mod) {
    if (!addr_table) {
        addr_table = AddrTable();

        addr_table_file = std::ofstream("out/x86_64-windows-msvc-debug/addr_table.txt");
        next_symbol_index = 0;

        for (const ir::Function *func : mod.get_functions()) {
            add_symbol(func->get_name(), mod);
        }

        for (const ir::FunctionDecl &external_func : mod.get_external_functions()) {
            add_symbol(external_func.get_name(), mod);
        }

        for (const ir::GlobalDecl &external_global : mod.get_external_globals()) {
            add_symbol(external_global.get_name(), mod);
        }
    } else {
        mod.add(ir::GlobalDecl("addr_table", ir::Primitive::I64));
    }

    for (ir::Function *func : mod.get_functions()) {
        replace_uses(func);
    }
}

void AddrTablePass::add_symbol(const std::string &name, ir::Module &mod) {
    unsigned index = next_symbol_index++;
    addr_table->insert(name, index);
    addr_table_file << name << '\n';

    ir::Global global(
        index == 0 ? "addr_table" : "addr_table_" + std::to_string(index),
        ir::Primitive::I64,
        ir::Value::from_global(name, ir::Primitive::ADDR)
    );
    global.set_external(true);
    mod.add(global);
}

void AddrTablePass::replace_uses(ir::Function *func) {
    for (ir::BasicBlock &basic_block : func->get_basic_blocks()) {
        replace_uses(func, basic_block);
    }
}

void AddrTablePass::replace_uses(ir::Function *func, ir::BasicBlock &basic_block) {
    for (ir::InstrIter iter = basic_block.begin(); iter != basic_block.end(); ++iter) {
        for (unsigned i = 0; i < iter->get_operands().size(); i++) {
            ir::Operand &operand = iter->get_operand(i);
            if (!operand.is_symbol()) {
                continue;
            }

            auto it = addr_table->find(operand.get_symbol_name());
            if (it == addr_table->end()) {
                continue;
            }

            unsigned index = it->second;

            ir::VirtualRegister ptr_reg = func->next_virtual_reg();
            ir::VirtualRegister reg = func->next_virtual_reg();

            basic_block.insert_before(
                iter,
                ir::Instruction(
                    ir::Opcode::ADD,
                    ptr_reg,
                    {ir::Operand::from_global("addr_table", ir::Primitive::ADDR),
                     ir::Operand::from_int_immediate(8 * index, ir::Primitive::I64)}
                )
            );

            basic_block.insert_before(
                iter,
                ir::Instruction(
                    ir::Opcode::LOAD,
                    reg,
                    {ir::Operand::from_type(ir::Primitive::ADDR),
                     ir::Operand::from_register(ptr_reg, ir::Primitive::ADDR)}
                )
            );

            operand = ir::Operand::from_register(reg, operand.get_type());
        }
    }
}

} // namespace passes
