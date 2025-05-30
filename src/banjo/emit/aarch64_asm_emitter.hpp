#ifndef BANJO_EMIT_AARCH64_ASM_EMITTER_H
#define BANJO_EMIT_AARCH64_ASM_EMITTER_H

#include "banjo/emit/emitter.hpp"
#include "banjo/target/target_description.hpp"

#include <unordered_map>

namespace banjo {

namespace codegen {

class AArch64AsmEmitter : public Emitter {

public:
    static const std::unordered_map<mcode::Opcode, std::string> OPCODE_NAMES;

private:
    target::TargetDescription target;
    std::string symbol_prefix;

public:
    AArch64AsmEmitter(mcode::Module &module, std::ostream &stream, target::TargetDescription target);
    void generate();

private:
    void emit_global(const mcode::Global &global);
    void emit_func(mcode::Function *func);
    void emit_basic_block(mcode::BasicBlock &basic_block);
    void emit_instr(mcode::Function *func, mcode::Instruction &instr);
    void emit_operand(mcode::Function *func, const mcode::Operand &operand);
    void emit_reg(int reg, int size);
    void emit_stack_slot(mcode::Function *func, int index);
    void emit_symbol(const mcode::Symbol &symbol);
    void emit_addr(const target::AArch64Address &addr);
    void emit_stack_slot_offset(mcode::Function *func, mcode::Operand::StackSlotOffset offset);
    void emit_condition(target::AArch64Condition condition);
};

} // namespace codegen

} // namespace banjo

#endif
