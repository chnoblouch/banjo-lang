#ifndef MCODE_CALLING_CONVENTION_H
#define MCODE_CALLING_CONVENTION_H

#include "banjo/ir/function.hpp"
#include "banjo/ir/type.hpp"
#include "banjo/mcode/instruction.hpp"
#include "banjo/mcode/parameter.hpp"
#include "banjo/mcode/register.hpp"
#include "banjo/mcode/stack_regions.hpp"

#include <algorithm>
#include <vector>

namespace banjo {

namespace target {
class TargetRegAnalyzer;
};

namespace codegen {
class IRLowerer;
};

namespace mcode {

class Function;
class StackFrame;

struct ArgStorage {
    bool in_reg;
    union {
        long reg;
        struct {
            unsigned arg_slot_index;
            long stack_offset;
        };
    };
};

class CallingConvention {

protected:
    std::vector<PhysicalReg> volatile_regs;

public:
    virtual ~CallingConvention() = default;

    const std::vector<PhysicalReg> &get_volatile_regs() const { return volatile_regs; }

    bool is_volatile(long reg) const {
        return std::find(volatile_regs.begin(), volatile_regs.end(), reg) != volatile_regs.end();
    }

    virtual void lower_call(codegen::IRLowerer &lowerer, ir::Instruction &instr) = 0;
    virtual void create_arg_store_region(StackFrame &frame, StackRegions &regions) = 0;
    virtual void create_call_arg_region(Function *func, StackFrame &frame, StackRegions &regions) = 0;
    virtual void create_implicit_region(Function *func, StackFrame &frame, StackRegions &regions) = 0;
    virtual int get_alloca_size(StackRegions &regions) = 0;
    virtual std::vector<Instruction> get_prolog(Function *func) = 0;
    virtual std::vector<Instruction> get_epilog(Function *func) = 0;

    virtual InstrIter fix_up_instr(BasicBlock &block, InstrIter iter, target::TargetRegAnalyzer &analyzer) {
        return iter;
    }

    virtual bool is_func_exit(Opcode opcode) = 0;
    virtual std::vector<ArgStorage> get_arg_storage(const std::vector<ir::Type> &types) = 0;
    virtual int get_implicit_stack_bytes(Function *func) = 0;
};

} // namespace mcode

} // namespace banjo

#endif
