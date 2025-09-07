#include "aarch64_instr_merge_pass.hpp"

#include "banjo/target/aarch64/aarch64_opcode.hpp"
#include "banjo/utils/timing.hpp"

#include <unordered_set>

namespace banjo {

namespace target {

const std::unordered_set<mcode::Opcode> REMOVABLE_INSTRS = {
    AArch64Opcode::ADD,
    AArch64Opcode::SUB,
    AArch64Opcode::MUL,
};

void AArch64InstrMergePass::run(mcode::Module &module_) {
    for (mcode::Function *func : module_.get_functions()) {
        run(func);
    }
}

void AArch64InstrMergePass::run(mcode::Function *func) {
    for (mcode::BasicBlock &basic_block : func->get_basic_blocks()) {
        run(basic_block);
    }
}

void AArch64InstrMergePass::run(mcode::BasicBlock &basic_block) {
    PROFILE_SCOPE("aarch64 instr merge");

    RegUsageMap usages = analyze_usages(basic_block);
    merge_instrs(basic_block, usages);
    remove_useless_instrs(basic_block, usages);
}

AArch64InstrMergePass::RegUsageMap AArch64InstrMergePass::analyze_usages(mcode::BasicBlock &basic_block) {
    RegUsageMap usages;

    for (mcode::InstrIter iter = basic_block.begin(); iter != basic_block.end(); ++iter) {
        // If the instruction has a destination, store the new producer.
        if (iter->has_dest() && iter->get_dest().is_virtual_reg()) {
            usages[iter->get_dest().get_virtual_reg()].producer = iter;
        }

        // For each register operand, increase the consumer count of the producer.
        for (unsigned i = 1; i < iter->get_operands().size(); i++) {
            mcode::Operand &operand = iter->get_operand(i);

            if (operand.is_virtual_reg()) {
                usages[operand.get_virtual_reg()].num_consumers++;
            }

            if (operand.is_aarch64_addr()) {
                const AArch64Address &addr = operand.get_aarch64_addr();
                if (addr.get_base().is_virtual()) {
                    usages[addr.get_base().get_virtual_reg()].num_consumers++;
                }
            }
        }
    }

    return usages;
}

void AArch64InstrMergePass::merge_instrs(mcode::BasicBlock &basic_block, RegUsageMap &usages) {
    for (mcode::Instruction &instr : basic_block) {
        switch (instr.get_opcode()) {
            case AArch64Opcode::STR: try_merge_str(instr, usages); break;
            case AArch64Opcode::ADD: try_merge_add(instr, usages); break;
            default: break;
        }
    }
}

void AArch64InstrMergePass::try_merge_str(mcode::Instruction &instr, RegUsageMap &usages) {
    mcode::Operand &m_dst = instr.get_operand(1);

    const AArch64Address &addr = m_dst.get_aarch64_addr();
    AArch64Address new_addr = try_merge_addr(addr, usages);
    m_dst = mcode::Operand::from_aarch64_addr(new_addr, m_dst.get_size());
}

void AArch64InstrMergePass::try_merge_add(mcode::Instruction &instr, RegUsageMap &usages) {
    if (!instr.get_operand(1).is_virtual_reg() || !instr.get_operand(2).is_int_immediate()) {
        return;
    }

    RegUsage &producer_usage = usages[instr.get_operand(1).get_virtual_reg()];
    mcode::Instruction &producer = *producer_usage.producer;

    if (producer.get_opcode() != AArch64Opcode::ADD || !producer.get_operand(2).is_stack_slot_offset()) {
        return;
    }

    mcode::Operand::StackSlotOffset new_offset = producer.get_operand(2).get_stack_slot_offset();
    new_offset.addend += instr.get_operand(2).get_int_immediate().to_s32();

    int size = instr.get_operand(2).get_size();
    instr.get_operand(1) = producer.get_operand(1);
    instr.get_operand(2) = mcode::Operand::from_stack_slot_offset(new_offset, size);
    producer_usage.num_consumers--;
}

AArch64Address AArch64InstrMergePass::try_merge_addr(const AArch64Address &addr, RegUsageMap &usages) {
    RegUsage &producer_usage = usages[addr.get_base().get_virtual_reg()];
    mcode::Instruction &producer = *producer_usage.producer;

    if (addr.get_type() == AArch64Address::Type::BASE && addr.get_base().is_virtual()) {
        if (producer.get_opcode() == AArch64Opcode::ADD) {
            if (producer.get_operand(2).is_int_immediate()) {
                mcode::Register new_base = producer.get_operand(1).get_register();
                int new_offset = producer.get_operand(2).get_int_immediate().to_s32();
                auto new_addr = AArch64Address::new_base_offset(new_base, new_offset);

                producer_usage.num_consumers--;
                return new_addr;
            }
        }
    }

    return addr;
}

void AArch64InstrMergePass::remove_useless_instrs(mcode::BasicBlock &basic_block, RegUsageMap &usages) {
    for (mcode::InstrIter iter = basic_block.begin(); iter != basic_block.end(); ++iter) {
        if (REMOVABLE_INSTRS.count(iter->get_opcode()) && iter->get_dest().is_virtual_reg()) {
            basic_block.remove(iter);
            continue;
        }

        const RegUsage &usage = usages[iter->get_dest().get_virtual_reg()];
        if (usage.num_consumers == 0) {
            basic_block.remove(iter);
        }
    }
}

} // namespace target

} // namespace banjo
