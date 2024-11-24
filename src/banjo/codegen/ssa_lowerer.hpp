#ifndef CODEGEN_SSA_LOWERER_H
#define CODEGEN_SSA_LOWERER_H

#include "banjo/mcode/module.hpp"
#include "banjo/ssa/module.hpp"
#include "banjo/target/target.hpp"


#include <unordered_map>
#include <vector>

namespace banjo {

namespace codegen {

struct IRLoweringContext {
    std::unordered_map<long, long> stack_regs;
    std::unordered_map<ssa::VirtualRegister, int> reg_use_counts;
};

class SSALowerer {

private:
    struct RegUsage {
        int def_index;
        int use_count;
    };

    struct BasicBlockContext {
        mcode::BasicBlock *basic_block;
        mcode::InstrIter insertion_iter;
        std::unordered_map<int, RegUsage> regs;
    };

    typedef std::unordered_map<ssa::BasicBlockIter, mcode::BasicBlockIter> BlockMap;

protected:
    target::Target *target;
    IRLoweringContext context;
    BasicBlockContext basic_block_context;

private:
    ssa::Module *module_;
    ssa::Function *func;
    ssa::BasicBlockIter basic_block_iter;
    ssa::InstrIter instr_iter;

    mcode::Module machine_module;
    mcode::Function *machine_func;
    mcode::BasicBlock *machine_basic_block;

public:
    SSALowerer(target::Target *target);
    virtual ~SSALowerer() = default;

    mcode::Module lower_module(ssa::Module &module_);

    const target::Target *get_target() const { return target; }

    ssa::Module &get_module() { return *module_; }
    ssa::Function &get_func() { return *func; }
    ssa::BasicBlockIter get_basic_block_iter() { return basic_block_iter; }
    ssa::BasicBlock &get_block() { return *basic_block_iter; }
    ssa::InstrIter &get_instr_iter() { return instr_iter; }
    mcode::Module &get_machine_module() { return machine_module; }
    mcode::Function *get_machine_func() { return machine_func; }
    mcode::BasicBlock &get_machine_basic_block() { return *machine_basic_block; }

    virtual mcode::Operand lower_value(const ssa::Operand &operand) = 0;
    virtual mcode::Operand lower_address(const ssa::Operand &operand) = 0;

    mcode::InstrIter emit(mcode::Instruction instr);
    mcode::Register lower_reg(ssa::VirtualRegister reg);

    unsigned get_size(const ssa::Type &type);
    unsigned get_alignment(const ssa::Type &type);
    unsigned get_member_offset(ssa::Structure *struct_, unsigned index);
    unsigned get_member_offset(const std::vector<ssa::Type> &types, unsigned index);
    mcode::Register create_tmp_reg();

    ssa::InstrIter get_producer(ssa::VirtualRegister reg);
    ssa::InstrIter get_producer_globally(ssa::VirtualRegister reg);
    unsigned get_num_uses(ssa::VirtualRegister reg);
    void discard_use(ssa::VirtualRegister reg);

    virtual mcode::CallingConvention *get_calling_convention(ssa::CallingConv calling_conv) = 0;

private:
    void lower_funcs();
    mcode::Parameter lower_param(mcode::ArgStorage storage, mcode::Function *machine_func);
    mcode::BasicBlock lower_basic_block(ssa::BasicBlock &basic_block);
    void store_graphs(ssa::Function &ir_func, mcode::Function *machine_func, const BlockMap &block_map);
    void lower_instr(ssa::Instruction &instr);
    void lower_globals();
    void lower_external_funcs();
    void lower_external_globals();
    void lower_dll_exports();

    void lower_alloca(ssa::Instruction &instr);

    virtual void init_module(ssa::Module &mod) {}
    virtual mcode::Value lower_global_value(ssa::Value &value) = 0;

    virtual void lower_load(ssa::Instruction &instr);
    virtual void lower_store(ssa::Instruction &instr);
    virtual void lower_loadarg(ssa::Instruction &instr);
    virtual void lower_add(ssa::Instruction &instr);
    virtual void lower_sub(ssa::Instruction &instr);
    virtual void lower_mul(ssa::Instruction &instr);
    virtual void lower_sdiv(ssa::Instruction &instr);
    virtual void lower_srem(ssa::Instruction &instr);
    virtual void lower_udiv(ssa::Instruction &instr);
    virtual void lower_urem(ssa::Instruction &instr);
    virtual void lower_fadd(ssa::Instruction &instr);
    virtual void lower_fsub(ssa::Instruction &instr);
    virtual void lower_fmul(ssa::Instruction &instr);
    virtual void lower_fdiv(ssa::Instruction &instr);
    virtual void lower_and(ssa::Instruction &instr);
    virtual void lower_or(ssa::Instruction &instr);
    virtual void lower_xor(ssa::Instruction &instr);
    virtual void lower_shl(ssa::Instruction &instr);
    virtual void lower_shr(ssa::Instruction &instr);
    virtual void lower_jmp(ssa::Instruction &instr);
    virtual void lower_cjmp(ssa::Instruction &instr);
    virtual void lower_fcjmp(ssa::Instruction &instr);
    virtual void lower_select(ssa::Instruction &instr);
    virtual void lower_call(ssa::Instruction &instr);
    virtual void lower_ret(ssa::Instruction &instr);
    virtual void lower_uextend(ssa::Instruction &instr);
    virtual void lower_sextend(ssa::Instruction &instr);
    virtual void lower_truncate(ssa::Instruction &instr);
    virtual void lower_fpromote(ssa::Instruction &instr);
    virtual void lower_fdemote(ssa::Instruction &instr);
    virtual void lower_utof(ssa::Instruction &instr);
    virtual void lower_stof(ssa::Instruction &instr);
    virtual void lower_ftou(ssa::Instruction &instr);
    virtual void lower_ftos(ssa::Instruction &instr);
    virtual void lower_offsetptr(ssa::Instruction &instr);
    virtual void lower_memberptr(ssa::Instruction &instr);
    virtual void lower_copy(ssa::Instruction &instr);
    virtual void lower_sqrt(ssa::Instruction &instr);
};

} // namespace codegen

} // namespace banjo

#endif
