#ifndef BANJO_EMIT_NASM_EMITTER_H
#define BANJO_EMIT_NASM_EMITTER_H

#include "banjo/emit/emitter.hpp"
#include "banjo/mcode/stack_frame.hpp"
#include "banjo/target/target_description.hpp"

#include <unordered_map>

namespace banjo {

namespace codegen {

class NASMEmitter : public Emitter {

private:
    target::TargetDescription target;

public:
    static const std::unordered_map<mcode::Opcode, std::string> OPCODE_NAMES;

    NASMEmitter(mcode::Module &module, std::ostream &stream, target::TargetDescription target);
    void generate();
    void emit_instr(mcode::BasicBlock &basic_block, mcode::Instruction &instr);

private:
    std::unordered_map<std::string, std::string> symbol_prefixes;

    void emit_func(mcode::Function *func);
    void gen_basic_block(mcode::BasicBlock &basic_block);

    std::string get_operand_name(mcode::BasicBlock &basic_block, mcode::Operand operand);
    std::string get_reg_name(mcode::BasicBlock &basic_block, mcode::Register reg, int size);
    std::string get_stack_slot_name(mcode::Function *func, mcode::StackSlotID stack_slot, bool brackets = true);
    std::string get_physical_reg_name(long reg, int size);
    std::string gen_symbol(const mcode::Symbol &symbol);
    std::string get_size_specifier(int size);
    std::string get_size_declaration(int size);
};

} // namespace codegen

} // namespace banjo

#endif
