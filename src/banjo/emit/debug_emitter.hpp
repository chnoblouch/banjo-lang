#ifndef DEBUG_EMITTER_H
#define DEBUG_EMITTER_H

#include "banjo/emit/emitter.hpp"
#include "banjo/target/target_description.hpp"

namespace banjo {

namespace codegen {

class DebugEmitter : public Emitter {

private:
    target::TargetDescription target;

public:
    DebugEmitter(mcode::Module &module, std::ostream &stream, target::TargetDescription target);
    void generate();

    static std::string instr_to_string(mcode::BasicBlock &basic_block, mcode::Instruction &instr);
    static std::string get_physical_reg_name(long reg, int size);

private:
    void generate(mcode::Function *func);
    void gen_basic_block(mcode::BasicBlock &basic_block);

    static std::string get_opcode_name(mcode::Opcode opcode);
    static std::string get_operand_name(mcode::BasicBlock &basic_block, mcode::Operand operand, int instr_index);
    static std::string get_reg_name(mcode::BasicBlock &basic_block, mcode::Register reg, int size, int instr_index);

    static std::string get_stack_slot_name(
        mcode::Function *func,
        mcode::Register reg,
        int instr_index,
        bool brackets = true
    );

    static std::string get_size_specifier(int size);
};

} // namespace codegen

} // namespace banjo

#endif
