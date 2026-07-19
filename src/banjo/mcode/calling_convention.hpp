#ifndef BANJO_MCODE_CALLING_CONVENTION_H
#define BANJO_MCODE_CALLING_CONVENTION_H

#include "banjo/mcode/function.hpp"
#include "banjo/mcode/instruction.hpp"
#include "banjo/mcode/register.hpp"
#include "banjo/mcode/stack_regions.hpp"
#include "banjo/ssa/function_type.hpp"
#include "banjo/ssa/instruction.hpp"

#include <algorithm>
#include <vector>

namespace banjo::target {
class TargetRegAnalyzer;
} // namespace banjo::target

namespace banjo::codegen {
class SSALowerer;
} // namespace banjo::codegen

namespace banjo::mcode {

class Function;
class StackFrame;

struct ArgStorage {
    bool in_reg;

    union {
        PhysicalReg reg;

        struct {
            unsigned arg_slot_index;
            unsigned stack_offset;
        };
    };
};

class CallingConvention {

protected:
    std::vector<PhysicalReg> volatile_regs;

public:
    virtual ~CallingConvention() = default;

    const std::vector<PhysicalReg> &get_volatile_regs() const { return volatile_regs; }

    bool is_volatile(PhysicalReg reg) const {
        return std::find(volatile_regs.begin(), volatile_regs.end(), reg) != volatile_regs.end();
    }

    virtual void lower_call(codegen::SSALowerer &lowerer, ssa::Instruction &instr) = 0;
    virtual void create_arg_store_region(StackFrame &frame, StackRegions &regions) = 0;
    virtual void create_call_arg_region(Function *func, StackFrame &frame, StackRegions &regions) = 0;
    virtual void create_implicit_region(Function *func, StackFrame &frame, StackRegions &regions) = 0;
    virtual int get_alloca_size(StackRegions &regions) = 0;
    virtual std::vector<Instruction> get_prolog(Function *func) = 0;
    virtual std::vector<Instruction> get_epilog(Function *func) = 0;
    virtual bool is_func_exit(Opcode opcode) = 0;
    virtual std::vector<ArgStorage> get_arg_storage(const ssa::FunctionType &func_type) = 0;
    virtual int get_implicit_stack_bytes(Function *func) = 0;
};

} // namespace banjo::mcode

#endif
