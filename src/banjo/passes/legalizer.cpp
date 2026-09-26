#include "legalizer.hpp"

#include "banjo/passes/pass_utils.hpp"
#include "banjo/ssa/builder.hpp"
#include "banjo/ssa/instruction.hpp"
#include "banjo/ssa/opcode.hpp"
#include "banjo/ssa/operand.hpp"
#include "banjo/ssa/primitive.hpp"

namespace banjo::passes {

Legalizer::Legalizer(target::Target *target) : Pass{"legalizer", target} {}

void Legalizer::run(ssa::Module &mod) {
    this->mod = &mod;

    for (ssa::Function *func : mod.get_functions()) {
        run(*func);
    }
}

void Legalizer::run(ssa::Function &func) {
    for (ssa::BasicBlockIter iter = func.begin(); iter != func.end(); ++iter) {
        run(func, iter);
    }
}

void Legalizer::run(ssa::Function &func, ssa::BasicBlockIter block) {
    ssa::InstrIter instr = block->get_instrs().get_first_iter();

    while (instr != block->get_instrs().end()) {
        ssa::InstrIter next = instr.get_next();

        switch (instr->get_opcode()) {
            case ssa::Opcode::LOAD: legalize_load(func, block, instr); break;
            case ssa::Opcode::STORE: legalize_store(func, block, instr); break;
            case ssa::Opcode::LOADARG: legalize_loadarg(func, block, instr); break;
            default: break;
        }

        instr = next;
    }

    ssa::InstrIter exit_instr = block->get_exit_iter();

    if (exit_instr->is_cond_branch()) {
        legalize_cjmp(func, block, exit_instr);
    } else if (exit_instr->get_opcode() == ssa::Opcode::RET) {
        legalize_ret(exit_instr);
    }
}

void Legalizer::legalize_load(ssa::Function &func, ssa::BasicBlockIter block, ssa::InstrIter instr) {
    ssa::Type type = instr->get_operand(0).get_type();
    unsigned size = get_target()->get_data_layout().get_size(type);

    if (size == 0) {
        PassUtils::replace_in_func(func, *instr->get_dest(), ssa::Operand::from_int_immediate(0, type));
        block->remove(instr);
        return;
    }

    ssa::Builder builder{func, *block};
    builder.set_cursor(instr.get_prev());

    if (size == 3) {
        ssa::Operand addr0 = instr->get_operand(1);
        ssa::Operand addr1 = builder.emit_offsetptr(addr0, 2, ssa::Primitive::U8);

        ssa::Operand value0 = builder.emit_load(ssa::Primitive::U16, addr0);
        ssa::Operand value1 = builder.emit_load(ssa::Primitive::U8, addr1);

        value0 = builder.emit_uextend(value0, ssa::Primitive::U32);
        value1 = builder.emit_uextend(value1, ssa::Primitive::U32);

        value1 = builder.emit_lshl(value1, 16);
        *instr = {ssa::Opcode::OR, *instr->get_dest(), {std::move(value0), std::move(value1)}};
    } else if (size == 5) {
        ssa::Operand addr0 = instr->get_operand(1);
        ssa::Operand addr1 = builder.emit_offsetptr(addr0, 4, ssa::Primitive::U8);

        ssa::Operand value0 = builder.emit_load(ssa::Primitive::U32, addr0);
        ssa::Operand value1 = builder.emit_load(ssa::Primitive::U8, addr1);

        value0 = builder.emit_uextend(value0, ssa::Primitive::U64);
        value1 = builder.emit_uextend(value1, ssa::Primitive::U64);

        value1 = builder.emit_lshl(value1, 32);
        *instr = {ssa::Opcode::OR, *instr->get_dest(), {std::move(value0), std::move(value1)}};
    } else if (size == 6) {
        ssa::Operand addr0 = instr->get_operand(1);
        ssa::Operand addr1 = builder.emit_offsetptr(addr0, 4, ssa::Primitive::U8);

        ssa::Operand value0 = builder.emit_load(ssa::Primitive::U32, addr0);
        ssa::Operand value1 = builder.emit_load(ssa::Primitive::U16, addr1);

        value0 = builder.emit_uextend(value0, ssa::Primitive::U64);
        value1 = builder.emit_uextend(value1, ssa::Primitive::U64);

        value1 = builder.emit_lshl(value1, 32);
        *instr = {ssa::Opcode::OR, *instr->get_dest(), {std::move(value0), std::move(value1)}};
    } else if (size == 7) {
        ssa::Operand addr0 = instr->get_operand(1);
        ssa::Operand addr1 = builder.emit_offsetptr(addr0, 4, ssa::Primitive::U8);
        ssa::Operand addr2 = builder.emit_offsetptr(addr0, 6, ssa::Primitive::U8);

        ssa::Operand value0 = builder.emit_load(ssa::Primitive::U32, addr0);
        ssa::Operand value1 = builder.emit_load(ssa::Primitive::U16, addr1);
        ssa::Operand value2 = builder.emit_load(ssa::Primitive::U8, addr2);

        value0 = builder.emit_uextend(value0, ssa::Primitive::U64);
        value1 = builder.emit_uextend(value1, ssa::Primitive::U64);
        value2 = builder.emit_uextend(value2, ssa::Primitive::U64);

        value1 = builder.emit_lshl(value1, 32);
        value2 = builder.emit_lshl(value2, 48);

        ssa::Operand or01 = builder.emit_or(std::move(value0), std::move(value1));
        *instr = {ssa::Opcode::OR, *instr->get_dest(), {std::move(or01), std::move(value2)}};
    } else {
        ASSERT(size == 1 || size == 2 || size == 4 || size == 8);
    }
}

void Legalizer::legalize_store(ssa::Function &func, ssa::BasicBlockIter block, ssa::InstrIter instr) {
    ssa::Type type = instr->get_operand(0).get_type();
    unsigned size = get_target()->get_data_layout().get_size(type);

    if (size == 0) {
        block->remove(instr);
        return;
    }

    ssa::Builder builder{func, *block};
    builder.set_cursor(instr);

    if (size == 3) {
        ssa::Operand addr0 = instr->get_operand(1);
        ssa::Operand addr1 = builder.emit_offsetptr(addr0, 2, ssa::Primitive::U8);

        ssa::Operand value0 = instr->get_operand(0).with_type(ssa::Primitive::U32);
        ssa::Operand value1 = builder.emit_lshr(value0, 16);

        value0 = builder.emit_truncate(value0, ssa::Primitive::U16);
        value1 = builder.emit_truncate(value1, ssa::Primitive::U8);

        builder.emit_store(value0, addr0);
        builder.emit_store(value1, addr1);

        block->remove(instr);
    } else if (size == 5) {
        ssa::Operand addr0 = instr->get_operand(1);
        ssa::Operand addr1 = builder.emit_offsetptr(addr0, 4, ssa::Primitive::U8);

        ssa::Operand value0 = instr->get_operand(0).with_type(ssa::Primitive::U64);
        ssa::Operand value1 = builder.emit_lshr(value0, 32);

        value0 = builder.emit_truncate(value0, ssa::Primitive::U32);
        value1 = builder.emit_truncate(value1, ssa::Primitive::U8);

        builder.emit_store(value0, addr0);
        builder.emit_store(value1, addr1);

        block->remove(instr);
    } else if (size == 6) {
        ssa::Operand addr0 = instr->get_operand(1);
        ssa::Operand addr1 = builder.emit_offsetptr(addr0, 4, ssa::Primitive::U8);

        ssa::Operand value0 = instr->get_operand(0).with_type(ssa::Primitive::U64);
        ssa::Operand value1 = builder.emit_lshr(value0, 32);

        value0 = builder.emit_truncate(value0, ssa::Primitive::U32);
        value1 = builder.emit_truncate(value1, ssa::Primitive::U16);

        builder.emit_store(value0, addr0);
        builder.emit_store(value1, addr1);

        block->remove(instr);
    } else if (size == 7) {
        ssa::Operand addr0 = instr->get_operand(1);
        ssa::Operand addr1 = builder.emit_offsetptr(addr0, 4, ssa::Primitive::U8);
        ssa::Operand addr2 = builder.emit_offsetptr(addr0, 6, ssa::Primitive::U8);

        ssa::Operand value0 = instr->get_operand(0).with_type(ssa::Primitive::U64);
        ssa::Operand value1 = builder.emit_lshr(value0, 32);
        ssa::Operand value2 = builder.emit_lshr(value0, 48);

        value0 = builder.emit_truncate(value0, ssa::Primitive::U32);
        value1 = builder.emit_truncate(value1, ssa::Primitive::U16);
        value2 = builder.emit_truncate(value2, ssa::Primitive::U8);

        builder.emit_store(value0, addr0);
        builder.emit_store(value1, addr1);
        builder.emit_store(value2, addr2);

        block->remove(instr);
    } else {
        ASSERT(size == 1 || size == 2 || size == 4 || size == 8);
    }
}

void Legalizer::legalize_loadarg(ssa::Function &func, ssa::BasicBlockIter block, ssa::InstrIter instr) {
    ssa::Type type = instr->get_operand(0).get_type();
    unsigned size = get_target()->get_data_layout().get_size(type);

    if (size == 0) {
        PassUtils::replace_in_func(func, *instr->get_dest(), ssa::Operand::from_int_immediate(0, type));
        block->remove(instr);
        return;
    }

    if (size == 3) {
        instr->get_operand(0).set_type(ssa::Primitive::U32);
    } else if (size == 5 || size == 6 || size == 7) {
        instr->get_operand(0).set_type(ssa::Primitive::U64);
    } else {
        ASSERT(size == 1 || size == 2 || size == 4 || size == 8);
    }
}

void Legalizer::legalize_cjmp(ssa::Function &func, ssa::BasicBlockIter block, ssa::InstrIter instr) {
    // Insert 'trampolines' for conditional branch instructions with the same
    // target but different arguments because the backends can't generate code
    // for them.

    ssa::BranchTarget &true_target = instr->get_operand(3).get_branch_target();
    ssa::BranchTarget &false_target = instr->get_operand(4).get_branch_target();

    if (true_target.block != false_target.block) {
        return;
    }

    // If both branches pass the same arguments, this entire comparison is
    // useless and we can branch directly to the target.
    if (true_target.args == false_target.args) {
        *instr = ssa::Instruction{ssa::Opcode::JMP, {ssa::Operand::from_branch_target(true_target)}};
        return;
    }

    ssa::BasicBlockIter new_true_target = func.insert_after(block, mod->next_block_label());
    ssa::BasicBlockIter new_false_target = func.insert_after(new_true_target, mod->next_block_label());

    new_true_target->append({ssa::Opcode::JMP, {ssa::Operand::from_branch_target(true_target)}});
    new_false_target->append({ssa::Opcode::JMP, {ssa::Operand::from_branch_target(false_target)}});

    true_target = ssa::BranchTarget{.block = new_true_target};
    false_target = ssa::BranchTarget{.block = new_false_target};
}

void Legalizer::legalize_ret(ssa::InstrIter instr) {
    // TODO: Update the actual return type of the function.

    if (instr->get_operands().empty()) {
        return;
    }

    ssa::Type type = instr->get_operand(0).get_type();
    unsigned size = get_target()->get_data_layout().get_size(type);

    if (size == 0) {
        *instr = ssa::Instruction{ssa::Opcode::RET};
    }
}

} // namespace banjo::passes
