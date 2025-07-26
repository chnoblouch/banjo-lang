#ifndef BANJO_TARGET_X86_64_CONST_LOWERING_H
#define BANJO_TARGET_X86_64_CONST_LOWERING_H

#include "banjo/mcode/operand.hpp"
#include "banjo/mcode/register.hpp"
#include "banjo/ssa/basic_block.hpp"
#include "banjo/ssa/instruction.hpp"


#include <map>
#include <string>
#include <unordered_map>

namespace banjo {

namespace target {

class X8664SSALowerer;

class X8664ConstLowering {

private:
    enum class ConstStorageAccess {
        LOAD,
        LOAD_INTO_REG,
        READ_REG,
    };

    struct ConstStorage {
        ConstStorageAccess access;
        std::string const_label;
        mcode::Register reg;
    };

    X8664SSALowerer &lowerer;

    unsigned cur_id = 0;
    std::map<float, std::string> const_f32s;

    std::unordered_map<ssa::InstrIter, std::map<float, ConstStorage>> f32_storage;
    ssa::BasicBlockIter last_block = nullptr;

public:
    X8664ConstLowering(X8664SSALowerer &lowerer);
    mcode::Value load_f32(float value);
    mcode::Value load_f64(double value);

private:
    void process_block();
    bool is_f32_used_later_on(float value, ssa::InstrIter user);
    bool is_discarding_instr(ssa::Opcode opcode);
};

} // namespace target

} // namespace banjo

#endif
