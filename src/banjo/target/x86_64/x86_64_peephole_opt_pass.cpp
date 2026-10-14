#include "x86_64_peephole_opt_pass.hpp"

#include "banjo/mcode/instruction.hpp"
#include "banjo/target/x86_64/x86_64_opcode.hpp"
#include "banjo/utils/timing.hpp"

namespace banjo::target {

void X8664PeepholeOptPass::run(mcode::Module &mod) {
    for (mcode::Function *func : mod.get_functions()) {
        run(*func);
    }
}

void X8664PeepholeOptPass::run(mcode::Function &func) {
    PROFILE_SCOPE("x86-64 peephole opt");

    for (mcode::BasicBlock &basic_block : func.basic_blocks) {
        for (mcode::Instruction &instr : basic_block) {
            run(instr);
        }
    }
}

void X8664PeepholeOptPass::run(mcode::Instruction &instr) {
    switch (instr.get_opcode()) {
        case X8664Opcode::MOV: optimize_mov(instr); break;
        case X8664Opcode::ADD: optimize_add(instr); break;
        case X8664Opcode::SUB: optimize_sub(instr); break;
        case X8664Opcode::AND: optimize_and(instr); break;
        case X8664Opcode::OR: optimize_or(instr); break;
        case X8664Opcode::XOR: optimize_xor(instr); break;
        case X8664Opcode::MOVSS: optimize_movss(instr); break;
        case X8664Opcode::MOVSD: optimize_movsd(instr); break;
    }
}

void X8664PeepholeOptPass::optimize_mov(mcode::Instruction &instr) {
    optimize_encoding(instr);
}

void X8664PeepholeOptPass::optimize_add(mcode::Instruction &instr) {
    optimize_encoding(instr);
}

void X8664PeepholeOptPass::optimize_sub(mcode::Instruction &instr) {
    optimize_encoding(instr);
}

void X8664PeepholeOptPass::optimize_and(mcode::Instruction &instr) {
    optimize_encoding(instr);
}

void X8664PeepholeOptPass::optimize_or(mcode::Instruction &instr) {
    optimize_encoding(instr);
}

void X8664PeepholeOptPass::optimize_xor(mcode::Instruction &instr) {
    optimize_encoding(instr);
}

void X8664PeepholeOptPass::optimize_movss(mcode::Instruction &instr) {
    if (instr.get_operand(0).is_register() && instr.get_operand(1).is_register()) {
        instr.set_opcode(X8664Opcode::MOVAPS);
    }
}

void X8664PeepholeOptPass::optimize_movsd(mcode::Instruction &instr) {
    if (instr.get_operand(0).is_register() && instr.get_operand(1).is_register()) {
        instr.set_opcode(X8664Opcode::MOVAPD);
    }
}

void X8664PeepholeOptPass::optimize_encoding(mcode::Instruction &instr) {
    // We generate 32-bit instructions for 8-bit and 16-bit operations because
    // they have a smaller encoding. Latency and throughput are equivalent on
    // modern processors.

    mcode::Operand &dst = instr.get_operand(0);
    mcode::Operand &src = instr.get_operand(1);

    if (dst.is_register() && (src.is_register() || src.is_int_immediate())) {
        unsigned size = dst.get_size();

        if (size == 1 || size == 2) {
            dst.set_size(4);
            src.set_size(4);
        }
    }
}

} // namespace banjo::target
