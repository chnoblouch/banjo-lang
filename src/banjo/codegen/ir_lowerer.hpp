#ifndef CODEGEN_IR_LOWERER_H
#define CODEGEN_IR_LOWERER_H

#include "ir/module.hpp"
#include "mcode/module.hpp"
#include "target/target.hpp"

#include <unordered_map>
#include <vector>

namespace banjo {

namespace codegen {

struct IRLoweringContext {
    std::unordered_map<long, long> stack_regs;
    std::unordered_map<ir::VirtualRegister, int> reg_use_counts;
};

class IRLowerer {

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

    typedef std::unordered_map<ir::BasicBlockIter, mcode::BasicBlockIter> BlockMap;

protected:
    target::Target *target;
    IRLoweringContext context;
    BasicBlockContext basic_block_context;

private:
    ir::Module *module_;
    ir::Function *func;
    ir::BasicBlockIter basic_block_iter;
    ir::InstrIter instr_iter;

    mcode::Module machine_module;
    mcode::Function *machine_func;
    mcode::BasicBlock *machine_basic_block;

public:
    IRLowerer(target::Target *target);
    virtual ~IRLowerer() = default;

    mcode::Module lower_module(ir::Module &module_);

    const target::Target *get_target() const { return target; }

    ir::Module &get_module() { return *module_; }
    ir::Function &get_func() { return *func; }
    ir::BasicBlockIter get_basic_block_iter() { return basic_block_iter; }
    ir::BasicBlock &get_block() { return *basic_block_iter; }
    ir::InstrIter &get_instr_iter() { return instr_iter; }
    mcode::Module &get_machine_module() { return machine_module; }
    mcode::Function *get_machine_func() { return machine_func; }
    mcode::BasicBlock &get_machine_basic_block() { return *machine_basic_block; }

    virtual mcode::Operand lower_value(const ir::Operand &operand) = 0;
    virtual mcode::Operand lower_address(const ir::Operand &operand) = 0;

    mcode::InstrIter emit(mcode::Instruction instr);
    mcode::Register lower_reg(ir::VirtualRegister reg);

    unsigned get_size(const ir::Type &type);
    unsigned get_alignment(const ir::Type &type);
    unsigned get_member_offset(ir::Structure *struct_, unsigned index);
    unsigned get_member_offset(const std::vector<ir::Type> &types, unsigned index);
    mcode::Register create_tmp_reg();

    ir::InstrIter get_producer(ir::VirtualRegister reg);
    ir::InstrIter get_producer_globally(ir::VirtualRegister reg);
    unsigned get_num_uses(ir::VirtualRegister reg);
    void discard_use(ir::VirtualRegister reg);

    virtual mcode::CallingConvention *get_calling_convention(ir::CallingConv calling_conv) = 0;

private:
    void lower_funcs();
    mcode::Parameter lower_param(mcode::ArgStorage storage, mcode::Function *machine_func);
    mcode::BasicBlock lower_basic_block(ir::BasicBlock &basic_block);
    void store_graphs(ir::Function &ir_func, mcode::Function *machine_func, const BlockMap &block_map);
    void lower_instr(ir::Instruction &instr);
    void lower_globals();
    void lower_external_funcs();
    void lower_external_globals();
    void lower_dll_exports();

    void lower_alloca(ir::Instruction &instr);

    virtual void init_module(ir::Module &mod) {}
    virtual mcode::Value lower_global_value(ir::Value &value) = 0;

    virtual void lower_load(ir::Instruction &instr);
    virtual void lower_store(ir::Instruction &instr);
    virtual void lower_loadarg(ir::Instruction &instr);
    virtual void lower_add(ir::Instruction &instr);
    virtual void lower_sub(ir::Instruction &instr);
    virtual void lower_mul(ir::Instruction &instr);
    virtual void lower_sdiv(ir::Instruction &instr);
    virtual void lower_srem(ir::Instruction &instr);
    virtual void lower_udiv(ir::Instruction &instr);
    virtual void lower_urem(ir::Instruction &instr);
    virtual void lower_fadd(ir::Instruction &instr);
    virtual void lower_fsub(ir::Instruction &instr);
    virtual void lower_fmul(ir::Instruction &instr);
    virtual void lower_fdiv(ir::Instruction &instr);
    virtual void lower_and(ir::Instruction &instr);
    virtual void lower_or(ir::Instruction &instr);
    virtual void lower_xor(ir::Instruction &instr);
    virtual void lower_shl(ir::Instruction &instr);
    virtual void lower_shr(ir::Instruction &instr);
    virtual void lower_jmp(ir::Instruction &instr);
    virtual void lower_cjmp(ir::Instruction &instr);
    virtual void lower_fcjmp(ir::Instruction &instr);
    virtual void lower_select(ir::Instruction &instr);
    virtual void lower_call(ir::Instruction &instr);
    virtual void lower_ret(ir::Instruction &instr);
    virtual void lower_uextend(ir::Instruction &instr);
    virtual void lower_sextend(ir::Instruction &instr);
    virtual void lower_truncate(ir::Instruction &instr);
    virtual void lower_fpromote(ir::Instruction &instr);
    virtual void lower_fdemote(ir::Instruction &instr);
    virtual void lower_utof(ir::Instruction &instr);
    virtual void lower_stof(ir::Instruction &instr);
    virtual void lower_ftou(ir::Instruction &instr);
    virtual void lower_ftos(ir::Instruction &instr);
    virtual void lower_offsetptr(ir::Instruction &instr);
    virtual void lower_memberptr(ir::Instruction &instr);
    virtual void lower_copy(ir::Instruction &instr);
    virtual void lower_sqrt(ir::Instruction &instr);
};

} // namespace codegen

} // namespace banjo

#endif
