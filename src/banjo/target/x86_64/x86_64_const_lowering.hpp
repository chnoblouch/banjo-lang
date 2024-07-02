#ifndef X86_64_CONST_LOWERING_H
#define X86_64_CONST_LOWERING_H

#include "codegen/ir_lowerer.hpp"

#include <map>
#include <optional>
#include <unordered_map>

namespace banjo {

namespace target {

class X8664ConstLowering {

private:
    enum class ConstStorageAccess { LOAD, LOAD_INTO_REG, READ_REG };

    struct ConstStorage {
        ConstStorageAccess access;
        std::string const_label;
        mcode::Register reg = mcode::Register::from_virtual(-1);
    };

    codegen::IRLowerer &lowerer;

    unsigned cur_id = 0;
    std::map<float, std::string> const_f32s;

    std::unordered_map<ir::InstrIter, std::map<float, ConstStorage>> f32_storage;
    ir::BasicBlockIter last_block = nullptr;

public:
    X8664ConstLowering(codegen::IRLowerer &lowerer);
    mcode::Value load_f32(float value);
    mcode::Value load_f64(double value);

private:
    void process_block();
    bool is_f32_used_later_on(float value, ir::InstrIter user);
    bool is_discarding_instr(ir::Opcode opcode);
};

} // namespace target

} // namespace banjo

#endif
