#ifndef BANJO_MCODE_INSTRUCTION_H
#define BANJO_MCODE_INSTRUCTION_H

#include "banjo/mcode/operand.hpp"
#include "banjo/mcode/register.hpp"
#include "banjo/utils/fixed_vector.hpp"
#include "banjo/utils/linked_list.hpp"

#include <utility>

namespace banjo {

namespace mcode {

typedef int Opcode;

namespace PseudoOpcode {
enum : Opcode {
    EH_PUSHREG = -1,
    EH_ALLOCSTACK = -2,
    EH_ENDPROLOG = -3,
};
}

enum RegUsage { DEF, USE, USE_DEF, KILL };

struct RegOp {
    Register reg;
    RegUsage usage;
};

class Instruction {

public:
    static constexpr unsigned MAX_NUM_OPERANDS = 4;
    typedef FixedVector<Operand, MAX_NUM_OPERANDS> OperandList;

    static constexpr unsigned FLAG_ARG_STORE = 1 << 0;
    static constexpr unsigned FLAG_ALLOCA = 1 << 1;
    static constexpr unsigned FLAG_CALL_ARG = 1 << 2;
    static constexpr unsigned FLAG_CALL = 1 << 3;
    static constexpr unsigned FLAG_FLOAT = 1 << 4;

private:
    Opcode opcode;
    OperandList operands;
    unsigned flags = 0;

public:
    Instruction() : opcode(-1), operands{} {}
    Instruction(Opcode opcode) : opcode(opcode), operands{} {}
    Instruction(Opcode opcode, OperandList operands) : opcode(opcode), operands(std::move(operands)) {}

    Instruction(Opcode opcode, OperandList operands, unsigned flag)
      : opcode(opcode),
        operands(std::move(operands)),
        flags(flag) {}

    Opcode get_opcode() { return opcode; }
    void set_opcode(Opcode opcode) { this->opcode = opcode; }

    OperandList &get_operands() { return operands; }
    Operand &get_operand(unsigned index) { return operands[index]; }
    Operand *try_get_operand(unsigned index) { return operands.get_size() > index ? &operands[index] : nullptr; }

    Operand &get_dest() { return operands[0]; }
    bool has_dest() { return operands.get_size() > 0; }

    unsigned get_flags() { return flags; }
    bool is_flag(unsigned flag) { return flags & flag; }
    void set_flag(unsigned flag) { flags |= flag; }
};

typedef LinkedListNode<Instruction> MachineInstrNode;
typedef LinkedListIter<Instruction> InstrIter;

} // namespace mcode

} // namespace banjo

#endif
