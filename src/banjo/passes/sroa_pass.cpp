#include "sroa_pass.hpp"

#include "banjo/ir/basic_block.hpp"
#include "banjo/ir/instruction.hpp"
#include "banjo/ir/structure.hpp"
#include "banjo/ir/virtual_register.hpp"
#include "banjo/passes/pass_utils.hpp"

#include <iostream>
#include <unordered_map>
#include <unordered_set>

namespace banjo {

namespace passes {

SROAPass::SROAPass(target::Target *target) : Pass("sroa", target) {}

void SROAPass::run(ir::Module &mod) {
    for (ir::Function *func : mod.get_functions()) {
        run(func);
    }
}

void SROAPass::run(ir::Function *func) {
    stack_values.clear();
    stack_ptr_defs.clear();

    for (ir::BasicBlockIter iter = func->begin(); iter != func->end(); ++iter) {
        collect_stack_values(iter);
    }

    std::unordered_map<ir::VirtualRegister, unsigned> root_ptr_defs = stack_ptr_defs;
    collect_stack_ptr_defs(*func);

    for (ir::BasicBlock &block : *func) {
        disable_invalid_splits(block);
    }

    // Recollect member pointers because some of them might have been invalidated
    // during the previous step.
    stack_ptr_defs = root_ptr_defs;
    collect_stack_ptr_defs(*func);

    for (StackValue &stack_value : stack_values) {
        if (!stack_value.parent) {
            split_root(stack_value, func);
        }
    }

    for (ir::BasicBlock &block : *func) {
        split_copies(func, block);
    }

    apply_splits(*func);

    dump(*func);
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

        StackValue value{
            .alloca_instr = iter,
            .alloca_block = block_iter,
            .type = type,
            .parent{},
        };

        unsigned index = stack_values.size();
        stack_values.push_back(value);
        stack_ptr_defs.insert({*iter->get_dest(), index});

        // Disable splitting if there is an array member.
        if (type.is_struct()) {
            for (const ir::StructureMember &member : type.get_struct()->get_members()) {
                if (member.type.get_array_length() != 1) {
                    continue;
                }
            }
        }

        collect_members(index);
    }
}

void SROAPass::collect_members(unsigned val_index) {
    ir::Type type = stack_values[val_index].type;
    stack_values[val_index].members = std::vector<unsigned>{};

    for (const ir::StructureMember &member : type.get_struct()->get_members()) {
        StackValue member_value{
            .alloca_instr = stack_values[val_index].alloca_instr,
            .alloca_block = stack_values[val_index].alloca_block,
            .type = member.type,
            .parent = val_index,
        };

        unsigned member_index = stack_values.size();
        stack_values.push_back(member_value);
        stack_values[val_index].members->push_back(member_index);

        if (is_aggregate(member.type)) {
            collect_members(member_index);
        }
    }
}

void SROAPass::disable_invalid_splits(ir::BasicBlock &block) {
    for (ir::InstrIter iter = block.begin(); iter != block.end(); ++iter) {
        // Memberptr instructions can be removed. Their destination register will be replaced
        // by the new replaced by the allocation during splitting.
        if (iter->get_opcode() == ir::Opcode::MEMBERPTR && iter->get_operand(1).is_register()) {
            continue;
        }

        // Copies can be replaced with loads and stores of the individual members.
        if (iter->get_opcode() == ir::Opcode::COPY) {
            continue;
        }

        // Disable splitting for all stack values that are used by this instruction.
        PassUtils::iter_regs(iter->get_operands(), [this](ir::VirtualRegister reg) {
            if (StackValue *value = look_up_stack_value(reg)) {
                value->members = {};
            }
        });
    }
}

void SROAPass::split_root(StackValue &value, ir::Function *func) {
    if (!value.members) {
        return;
    }

    for (unsigned member_index : *value.members) {
        split_member(stack_values[member_index], func);
    }

    value.alloca_block->remove(value.alloca_instr);
}

void SROAPass::split_member(StackValue &value, ir::Function *func) {
    if (value.members) {
        for (unsigned member_index : *value.members) {
            split_member(stack_values[member_index], func);
        }
    } else {
        ir::VirtualRegister reg = func->next_virtual_reg();
        ir::Operand operand = ir::Operand::from_type(value.type);
        value.alloca_block->insert_before(value.alloca_instr, ir::Instruction(ir::Opcode::ALLOCA, reg, {operand}));
        value.replacement = reg;
    }
}

void SROAPass::collect_stack_ptr_defs(ir::Function &func) {
    PassUtils::UseMap use_map = PassUtils::collect_uses(func);

    for (auto &[reg, value_index] : stack_ptr_defs) {
        StackValue &value = stack_values[value_index];

        if (value.parent || !value.members) {
            continue;
        }

        collect_member_ptr_defs(use_map, reg, value);
    }
}

void SROAPass::collect_member_ptr_defs(PassUtils::UseMap &use_map, ir::VirtualRegister base, StackValue &value) {
    for (ir::InstrIter use : use_map[base]) {
        if (use->get_opcode() != ir::Opcode::MEMBERPTR) {
            continue;
        }

        ir::VirtualRegister dst = *use->get_dest();
        unsigned index = use->get_operand(2).get_int_immediate().to_u64();

        unsigned member_value_index = (*value.members)[index];
        StackValue &member = stack_values[member_value_index];
        stack_ptr_defs.insert({dst, member_value_index});

        if (member.members) {
            collect_member_ptr_defs(use_map, dst, member);
        }
    }
}

void SROAPass::split_copies(ir::Function *func, ir::BasicBlock &block) {
    for (ir::InstrIter iter = block.begin(); iter != block.end(); ++iter) {
        if (iter->get_opcode() != ir::Opcode::COPY) {
            continue;
        }

        const ir::Operand &dst = iter->get_operand(0);
        const ir::Operand &src = iter->get_operand(1);
        const ir::Type &type = iter->get_operand(2).get_type();

        if (!is_aggregate(type)) {
            continue;
        }

        ir::VirtualRegister dst_reg = dst.get_register();
        ir::VirtualRegister src_reg = src.get_register();

        InsertionContext ctx{
            .func = func,
            .block = block,
            .instr = iter,
        };

        Ref dst_ref{
            .ptr = dst_reg,
            .stack_value = look_up_stack_value_index(dst_reg),
        };
        Ref src_ref{
            .ptr = src_reg,
            .stack_value = look_up_stack_value_index(src_reg),
        };

        copy_members(ctx, dst_ref, src_ref, type);

        ir::InstrIter prev = iter.get_prev();
        block.remove(iter);
        iter = prev;
    }
}

void SROAPass::copy_members(InsertionContext &ctx, Ref dst, Ref src, const ir::Type &type) {
    for (unsigned i = 0; i < type.get_struct()->get_members().size(); i++) {
        const ir::Type &member_type = type.get_struct()->get_members()[i].type;

        Ref member_dst = create_member_pointer(ctx, dst, type, i);
        Ref member_src = create_member_pointer(ctx, src, type, i);

        if (is_aggregate(member_type)) {
            copy_members(ctx, member_dst, member_src, member_type);
            continue;
        }

        ir::VirtualRegister tmp_reg = ctx.func->next_virtual_reg();

        // clang-format off
        ctx.block.insert_before(ctx.instr, ir::Instruction(ir::Opcode::LOAD, tmp_reg, {
            ir::Operand::from_type(member_type),
            ir::Operand::from_register(*member_src.ptr, ir::Primitive::ADDR)
        }));
        ctx.block.insert_before(ctx.instr, ir::Instruction(ir::Opcode::STORE, {
            ir::Operand::from_register(tmp_reg, member_type),
            ir::Operand::from_register(*member_dst.ptr, ir::Primitive::ADDR)
        }));
        // clang-format on
    }
}

SROAPass::Ref SROAPass::create_member_pointer(
    InsertionContext &ctx,
    Ref ref,
    const ir::Type &parent_type,
    unsigned index
) {
    if (ref.stack_value) {
        StackValue &value = stack_values[*ref.stack_value];

        if (value.members) {
            unsigned member_index = (*value.members)[index];
            StackValue &member = stack_values[member_index];

            return Ref{
                .ptr = member.replacement,
                .stack_value = member_index,
            };
        }
    }

    ir::VirtualRegister ptr = ctx.func->next_virtual_reg();

    // clang-format off
    ctx.block.insert_before(ctx.instr, ir::Instruction(ir::Opcode::MEMBERPTR, ptr, {
        ir::Operand::from_type(parent_type),
        ir::Operand::from_register(*ref.ptr, ir::Primitive::ADDR),
        ir::Operand::from_int_immediate(index)
    }));
    // clang-format on

    return Ref{
        .ptr = ptr,
    };
}

void SROAPass::apply_splits(ir::Function &func) {
    for (ir::BasicBlock &block : func) {
        for (ir::InstrIter iter = block.begin(); iter != block.end(); ++iter) {
            if (iter->get_opcode() != ir::Opcode::MEMBERPTR) {
                continue;
            }

            ir::VirtualRegister dst = *iter->get_dest();
            StackValue *value = look_up_stack_value(dst);

            if (!value) {
                continue;
            }

            if (value->replacement) {
                PassUtils::replace_in_func(func, dst, *value->replacement);
            }

            ir::InstrIter prev = iter.get_prev();
            block.remove(iter);
            iter = prev;
        }
    }
}

SROAPass::StackValue *SROAPass::look_up_stack_value(ir::VirtualRegister reg) {
    if (std::optional<unsigned> index = look_up_stack_value_index(reg)) {
        return &stack_values[*index];
    } else {
        return nullptr;
    }
}

std::optional<unsigned> SROAPass::look_up_stack_value_index(ir::VirtualRegister reg) {
    auto iter = stack_ptr_defs.find(reg);
    return iter == stack_ptr_defs.end() ? std::optional<unsigned>{} : iter->second;
}

bool SROAPass::is_aggregate(const ir::Type &type) {
    return type.is_struct() && type.get_array_length() == 1;
}

void SROAPass::dump(ir::Function &func) {
    std::cout << "\n  stack values for function `" << func.get_name() << "`:\n\n";
    dump_stack_values();

    std::cout << "  stack pointer replacements: \n\n";
    dump_stack_replacements();
}

void SROAPass::dump_stack_values() {
    for (const auto &[reg, value_index] : stack_ptr_defs) {
        StackValue &stack_value = stack_values[value_index];

        if (stack_value.parent) {
            continue;
        }

        dump_stack_value(reg, stack_value, 0);
        std::cout << "\n";
    }
}

void SROAPass::dump_stack_value(std::optional<ir::VirtualRegister> reg, StackValue &value, unsigned indent) {
    std::cout << "    ";

    if (reg) {
        std::cout << "%" << *reg << ": ";
    }

    std::cout << std::string(2 * indent, ' ');

    if (!value.members) {
        std::cout << "unsplittable";

        if (value.replacement) {
            std::cout << " -> %" << *value.replacement;
        }

        std::cout << '\n';
        return;
    }

    if (value.type.is_struct()) std::cout << value.type.get_struct()->get_name();
    std::cout << " {\n";

    for (unsigned member_index : *value.members) {
        dump_stack_value({}, stack_values[member_index], indent + 1);
    }

    std::cout << "    " << std::string(2 * indent, ' ') << "}";

    if (value.members && value.replacement) {
        std::cout << " -> %" << *value.replacement;
    }

    std::cout << '\n';
}

void SROAPass::dump_stack_replacements() {
    for (auto &[old_reg, value_index] : stack_ptr_defs) {
        StackValue &value = stack_values[value_index];

        std::cout << "    %" << old_reg << " -> ";

        if (value.replacement) {
            std::cout << "%" << *value.replacement;
        } else {
            std::cout << "removed";
        }

        std::cout << "\n";
    }

    std::cout << "\n";
}

} // namespace passes

} // namespace banjo
