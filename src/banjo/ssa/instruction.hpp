#ifndef BANJO_SSA_INSTRUCTION_H
#define BANJO_SSA_INSTRUCTION_H

#include "banjo/ssa/opcode.hpp"
#include "banjo/ssa/operand.hpp"
#include "banjo/ssa/virtual_register.hpp"
#include "banjo/utils/linked_list.hpp"

#include <cstdint>
#include <optional>
#include <utility>
#include <vector>

namespace banjo {

namespace ssa {

class Instruction {

public:
    enum class Attribute : std::uint8_t {
        NONE,
        ARG_STORE,
        SAVE_ARG,
        VARIADIC,
    };

private:
    Opcode opcode;
    std::optional<VirtualRegister> dest;
    std::vector<Operand> operands;
    Attribute attr = Attribute::NONE;
    unsigned attr_data;

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
    std::optional<VirtualRegister> &get_dest() { return dest; }
    std::vector<Operand> &get_operands() { return operands; }
    Operand &get_operand(int index) { return operands[index]; }
    Attribute get_attr() const { return attr; }
    unsigned get_attr_data() const { return attr_data; }

    void set_opcode(ssa::Opcode opcode) { this->opcode = opcode; }
    void set_dest(VirtualRegister dest) { this->dest = std::optional{dest}; }
    void set_attr(Attribute attr) { this->attr = attr; }
    void set_attrs_data(unsigned attr_data) { this->attr_data = attr_data; }

    bool is_branching() const {
        return opcode == ssa::Opcode::JMP || opcode == ssa::Opcode::CJMP || opcode == ssa::Opcode::FCJMP;
    }
};

typedef LinkedListNode<Instruction> InstrNode;
typedef LinkedListIter<Instruction> InstrIter;

} // namespace ssa

} // namespace banjo

#endif
