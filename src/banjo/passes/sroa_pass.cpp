#include "sroa_pass.hpp"

#include "ir/structure.hpp"
#include "passes/pass_utils.hpp"

#include <iostream>

namespace passes {

SROAPass::SROAPass(target::Target *target) : Pass("sroa", target) {}

void SROAPass::run(ir::Module &mod) {
    for (ir::Function *func : mod.get_functions()) {
        run(func, mod);
    }
}

void SROAPass::run(ir::Function *func, ir::Module &mod) {
    stack_values.clear();
    roots.clear();
    stack_ptr_defs.clear();

    for (ir::BasicBlockIter iter = func->begin(); iter != func->end(); ++iter) {
        collect_stack_values(iter);
    }

    for (ir::BasicBlock &block : *func) {
        collect_uses(block);
    }

    for (auto iter : roots) {
        split_root(stack_values[iter.second], func);
    }

    for (ir::BasicBlock &block : *func) {
        split_copies(func, block, mod);
    }

    for (ir::BasicBlock &block : *func) {
        apply_splits(block);
    }

    // std::cout << "--- STACK VALUES FOR " << func->get_name() << " ---" << std::endl;
    // dump_stack_values();
}

void SROAPass::collect_stack_values(ir::BasicBlockIter block_iter) {
    for (ir::InstrIter iter = block_iter->begin(); iter != block_iter->end(); ++iter) {
        if (iter->get_opcode() != ir::Opcode::ALLOCA) {
            continue;
        }

        const ir::Type &type = iter->get_operand(0).get_type();
        if (!is_aggregate(type)) {
            continue;
        }

        StackValue val{
            .alloca_instr = iter,
            .alloca_block = block_iter,
            .type = type,
            .splittable = is_splitting_possible(type),
        };

        unsigned index = stack_values.size();
        stack_values.push_back(val);
        roots.insert({*iter->get_dest(), index});

        collect_members(index);
    }
}

bool SROAPass::is_splitting_possible(const ir::Type &type) {
    if (!type.is_struct()) {
        return false;
    }

    for (const ir::StructureMember &member : type.get_struct()->get_members()) {
        if (member.type.get_array_length() != 1) {
            return false;
        }

        if (is_aggregate(member.type) && !is_splitting_possible(member.type)) {
            return false;
        }
    }

    return true;
}

void SROAPass::collect_members(unsigned val_index) {
    ir::Type val_type = stack_values[val_index].type;

    for (const ir::StructureMember &member : val_type.get_struct()->get_members()) {
        StackValue member_val{
            .alloca_instr = stack_values[val_index].alloca_instr,
            .alloca_block = stack_values[val_index].alloca_block,
            .type = member.type,
            .parent = val_index,
            .splittable = is_splitting_possible(member.type),
        };

        unsigned member_index = stack_values.size();
        stack_values.push_back(member_val);
        stack_values[val_index].members.push_back(member_index);

        if (is_aggregate(member.type)) {
            collect_members(member_index);
        }
    }
}

void SROAPass::collect_uses(ir::BasicBlock &block) {
    for (ir::InstrIter iter = block.begin(); iter != block.end(); ++iter) {
        if (iter->get_opcode() == ir::Opcode::MEMBERPTR && iter->get_operand(1).is_register()) {
            analyze_memberptr(iter);
            continue;
        }

        // We can't split a root if a value is loaded from or stored to it.
        if (iter->get_opcode() == ir::Opcode::LOAD || iter->get_opcode() == ir::Opcode::STORE) {
            PassUtils::iter_regs(iter->get_operands(), [this](ir::VirtualRegister reg) {
                if (roots.contains(reg)) {
                    disable_splitting(stack_values[roots[reg]]);
                } else if (stack_ptr_defs.contains(reg)) {
                    disable_splitting(stack_values[stack_ptr_defs[reg]]);
                }
            });
            continue;
        }

        // Copies can be replaced with loads and stores of the individual members
        if (iter->get_opcode() == ir::Opcode::COPY) {
            continue;
        }

        PassUtils::iter_regs(iter->get_operands(), [this](ir::VirtualRegister reg) {
            if (stack_ptr_defs.contains(reg)) {
                disable_splitting(stack_values[stack_ptr_defs[reg]]);
            }

            if (roots.contains(reg)) {
                disable_splitting(stack_values[roots[reg]]);
            }
        });
    }
}

void SROAPass::disable_splitting(StackValue &value) {
    value.splittable = false;

    for (unsigned member_index : value.members) {
        disable_splitting(stack_values[member_index]);
    }
}

void SROAPass::analyze_memberptr(ir::InstrIter iter) {
    const ir::Type &type = iter->get_operand(0).get_type();
    ir::VirtualRegister src = iter->get_operand(1).get_register();
    unsigned member_index = iter->get_operand(2).get_int_immediate().to_u64();
    ir::VirtualRegister dst = *iter->get_dest();

    StackValue *val = nullptr;

    auto root_iter = roots.find(src);
    if (root_iter != roots.end()) {
        val = &stack_values.at(root_iter->second);
    } else {
        auto stack_ptr_iter = stack_ptr_defs.find(src);
        if (stack_ptr_iter != stack_ptr_defs.end()) {
            val = &stack_values.at(stack_ptr_iter->second);
        } else {
            return;
        }
    }

    // The type may not match if there was a pointer cast between the definition and the memberptr instruction.
    if (type != val->type) {
        return;
    }

    stack_ptr_defs[dst] = val->members.at(member_index);
}

void SROAPass::split_root(StackValue &value, ir::Function *func) {
    if (!value.splittable || !is_aggregate(value.type)) {
        return;
    }

    for (unsigned member_index : value.members) {
        split_member(stack_values[member_index], func);
    }

    value.alloca_block->remove(value.alloca_instr);
}

void SROAPass::split_member(StackValue &value, ir::Function *func) {
    if (value.splittable && is_aggregate(value.type)) {
        for (unsigned member_index : value.members) {
            split_member(stack_values[member_index], func);
        }
    } else {
        ir::VirtualRegister reg = func->next_virtual_reg();
        ir::Operand operand = ir::Operand::from_type(value.type);
        value.alloca_block->insert_before(value.alloca_instr, ir::Instruction(ir::Opcode::ALLOCA, reg, {operand}));
        value.split_alloca = reg;
    }
}

void SROAPass::split_copies(ir::Function *func, ir::BasicBlock &block, ir::Module &mod) {
    for (ir::InstrIter iter = block.begin(); iter != block.end(); ++iter) {
        if (iter->get_opcode() != ir::Opcode::COPY) {
            continue;
        }

        const ir::Operand &dst = iter->get_operand(0);
        const ir::Operand &src = iter->get_operand(1);
        const ir::Type &type = iter->get_operand(2).get_type();

        if (!is_aggregate(type) || src.get_type() != dst.get_type()) {
            continue;
        }

        ir::VirtualRegister dst_reg = dst.get_register();
        ir::VirtualRegister src_reg = src.get_register();

        InsertionContext ctx{.mod = mod, .func = func, .block = block, .instr = iter};
        Ref dst_ref{.ptr = dst_reg, .stack_val = find_stack_val(dst_reg)};
        Ref src_ref{.ptr = src_reg, .stack_val = find_stack_val(src_reg)};

        copy_members(ctx, dst_ref, src_ref, type);

        ir::InstrIter prev = iter.get_prev();
        block.remove(iter);
        iter = prev;
    }
}

std::optional<unsigned> SROAPass::find_stack_val(ir::VirtualRegister reg) {
    if (roots.contains(reg)) {
        return roots[reg];
    } else if (stack_ptr_defs.contains(reg)) {
        return stack_ptr_defs[reg];
    } else {
        return {};
    }
}

void SROAPass::copy_members(InsertionContext &ctx, Ref dst, Ref src, const ir::Type &type) {
    // TODO: cache this somewhere
    if (!is_splitting_possible(type)) {
        // clang-format off
        ctx.block.insert_before(ctx.instr, ir::Instruction(ir::Opcode::COPY, {
            ir::Operand::from_register(dst.ptr, ir::Primitive::ADDR),
            ir::Operand::from_register(src.ptr, ir::Primitive::ADDR),
            ir::Operand::from_type(type),
        }));
        // clang-format on

        return;
    }

    for (unsigned i = 0; i < type.get_struct()->get_members().size(); i++) {
        const ir::Type &member_type = type.get_struct()->get_members()[i].type;

        Ref member_dst = get_final_memberptr(ctx, dst, type, i);
        Ref member_src = get_final_memberptr(ctx, src, type, i);

        if (is_aggregate(member_type)) {
            copy_members(ctx, member_dst, member_src, member_type);
            return;
        }

        ir::VirtualRegister tmp_reg = ctx.func->next_virtual_reg();

        // clang-format off
        ctx.block.insert_before(ctx.instr, ir::Instruction(ir::Opcode::LOAD, tmp_reg, {
            ir::Operand::from_type(member_type),
            ir::Operand::from_register(member_src.ptr, ir::Primitive::ADDR)
        }));
        ctx.block.insert_before(ctx.instr, ir::Instruction(ir::Opcode::STORE, {
            ir::Operand::from_register(tmp_reg, member_type),
            ir::Operand::from_register(member_dst.ptr, ir::Primitive::ADDR)
        }));
        // clang-format on
    }
}

SROAPass::Ref SROAPass::get_final_memberptr(
    InsertionContext &ctx,
    Ref ref,
    const ir::Type &parent_type,
    unsigned index
) {
    std::optional<unsigned> val_index = find_stack_val(ref.ptr);

    if (val_index) {
        StackValue &val = stack_values[*val_index];
        if (val.split_alloca) {
            return Ref{.ptr = *val.split_alloca, .stack_val = *val_index};
        }
    }

    ir::VirtualRegister ptr = ctx.func->next_virtual_reg();

    // clang-format off
    ctx.block.insert_before(ctx.instr, ir::Instruction(ir::Opcode::MEMBERPTR, ptr, {
        ir::Operand::from_type(parent_type),
        ir::Operand::from_register(ref.ptr, ir::Primitive::ADDR),
        ir::Operand::from_int_immediate(index)
    }));
    // clang-format on

    // Is this a member of a stack value?
    if (ref.stack_val) {
        // We have just created a new stack pointer def that can be removed during rewriting.
        const StackValue &val = stack_values[*ref.stack_val];
        unsigned member_val = val.members[index];
        stack_ptr_defs[ptr] = member_val;

        return Ref{.ptr = ptr, .stack_val = member_val};
    } else {
        return Ref{.ptr = ptr};
    }
}

void SROAPass::apply_splits(ir::BasicBlock &block) {
    for (ir::InstrIter iter = block.begin(); iter != block.end(); ++iter) {
        if (iter->get_opcode() != ir::Opcode::MEMBERPTR) {
            continue;
        }

        ir::VirtualRegister dst = *iter->get_dest();
        if (!stack_ptr_defs.contains(dst)) {
            continue;
        }

        StackValue &value = stack_values[stack_ptr_defs[dst]];
        if (!stack_values[value.parent].splittable) {
            continue;
        }

        if (value.split_alloca) {
            PassUtils::replace_in_block(block, dst, *value.split_alloca);
        }

        ir::InstrIter prev = iter.get_prev();
        block.remove(iter);
        iter = prev;
    }
}

bool SROAPass::is_aggregate(const ir::Type &type) {
    return type.is_struct() && type.get_array_length() == 1;
}

void SROAPass::dump_stack_values() {
    for (auto iter : roots) {
        std::cout << "%" << iter.first << ": ";
        dump_stack_value(stack_values[iter.second], 0);
        std::cout << "\n";
    }
}

void SROAPass::dump_stack_value(StackValue &value, unsigned indent) {
    std::cout << std::string(2 * indent, ' ') << (value.splittable ? "+ " : "x ");

    if (!is_aggregate(value.type)) {
        std::cout << "member";
        if (value.split_alloca) {
            std::cout << " -> %" << *value.split_alloca;
        }

        std::cout << '\n';
        return;
    }

    if (value.type.is_struct()) std::cout << value.type.get_struct()->get_name();
    std::cout << " {\n";

    for (unsigned member_index : value.members) {
        dump_stack_value(stack_values[member_index], indent + 1);
    }

    std::cout << std::string(2 * indent, ' ') << "}\n";
}

} // namespace passes
