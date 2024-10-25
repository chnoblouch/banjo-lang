#ifndef IR_INSTRUCTION_H
#define IR_INSTRUCTION_H

#include "banjo/ir/opcode.hpp"
#include "banjo/ir/operand.hpp"
#include "banjo/ir/type.hpp"
#include "banjo/ir/virtual_register.hpp"
#include "banjo/utils/linked_list.hpp"

#include <optional>
#include <utility>
#include <vector>

namespace banjo {

namespace ir {

class Instruction {

public:
    static constexpr unsigned FLAG_ARG_STORE = 1 << 0;
    static constexpr unsigned FLAG_SAVE_ARG = 1 << 1;

private:
    Opcode opcode;
    std::optional<VirtualRegister> dest;
    std::vector<Operand> operands;
    unsigned flags = 0;

public:
    Instruction() {}

    Instruction(Opcode opcode, std::optional<VirtualRegister> dest, std::vector<Operand> operands)
      : opcode(opcode),
        dest(dest),
        operands(std::move(operands)) {}

    Instruction(Opcode opcode, VirtualRegister dest, std::vector<Operand> operands)
      : opcode(opcode),
        dest{dest},
        operands(std::move(operands)) {}

    Instruction(Opcode opcode, std::vector<Operand> operands) : opcode(opcode), dest{}, operands(std::move(operands)) {}

    Instruction(Opcode opcode) : opcode(opcode), dest{}, operands{} {}

    Opcode get_opcode() const { return opcode; }
    void set_opcode(ir::Opcode opcode) { this->opcode = opcode; }

    std::optional<VirtualRegister> &get_dest() { return dest; }
    void set_dest(VirtualRegister dest) { this->dest = std::optional{dest}; }

    std::vector<Operand> &get_operands() { return operands; }
    Operand &get_operand(int index) { return operands[index]; }

    unsigned get_flags() const { return flags; }
    bool is_flag(unsigned flag) const { return flags & flag; }
    void set_flag(unsigned flag) { flags |= flag; }

    bool is_branching() const {
        return opcode == ir::Opcode::JMP || opcode == ir::Opcode::CJMP || opcode == ir::Opcode::FCJMP;
    }
};

typedef LinkedListNode<Instruction> InstrNode;
typedef LinkedListIter<Instruction> InstrIter;

} // namespace ir

} // namespace banjo

#endif
