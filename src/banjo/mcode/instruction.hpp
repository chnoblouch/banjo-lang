#ifndef MCODE_INSTRUCTION_H
#define MCODE_INSTRUCTION_H

#include "mcode/operand.hpp"
#include "mcode/register.hpp"
#include "utils/fixed_vector.hpp"
#include "utils/linked_list.hpp"

#include <utility>
#include <vector>

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
    static constexpr int MAX_NUM_OPERANDS = 4;
    typedef FixedVector<Operand, MAX_NUM_OPERANDS> OperandList;

    static constexpr unsigned FLAG_ARG_STORE = 1 << 0;
    static constexpr unsigned FLAG_ALLOCA = 1 << 1;
    static constexpr unsigned FLAG_CALL_ARG = 1 << 2;
    static constexpr unsigned FLAG_CALL = 1 << 3;
    static constexpr unsigned FLAG_FLOAT = 1 << 4;

private:
    Opcode opcode;
    OperandList operands;
    std::vector<RegOp> reg_ops;
    unsigned number;
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
    Operand &get_operand(int index) { return operands[index]; }

    Operand &get_dest() { return operands[0]; }
    bool has_dest() { return operands.get_size() > 0; }
    void set_dest(Operand dest) { operands[0] = dest; }

    void add_reg_op(PhysicalReg reg, RegUsage usage) {
        reg_ops.push_back(RegOp{.reg = Register::from_physical(reg), .usage = usage});
    }

    std::vector<RegOp> &get_reg_ops() { return reg_ops; }

    unsigned get_number() const { return number; }
    void set_number(unsigned number) { this->number = number; }

    unsigned get_flags() { return flags; }
    bool is_flag(unsigned flag) { return flags & flag; }
    void set_flag(unsigned flag) { flags |= flag; }
};

typedef LinkedListNode<Instruction> MachineInstrNode;
typedef LinkedListIter<Instruction> InstrIter;

} // namespace mcode

#endif
