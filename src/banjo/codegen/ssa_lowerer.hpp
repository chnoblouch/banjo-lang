#ifndef BANJO_CODEGEN_SSA_LOWERER_H
#define BANJO_CODEGEN_SSA_LOWERER_H

#include "banjo/codegen/instr_context.hpp"
#include "banjo/mcode/basic_block.hpp"
#include "banjo/mcode/calling_convention.hpp"
#include "banjo/mcode/module.hpp"
#include "banjo/mcode/stack_frame.hpp"
#include "banjo/ssa/module.hpp"
#include "banjo/ssa/virtual_register.hpp"
#include "banjo/target/target.hpp"

#include <unordered_map>
#include <variant>

namespace banjo::codegen {

class SSALowerer {

private:
    struct RegUsage {
        int def_index;
        int use_count;
    };

public:
    struct RegOffset {
        mcode::Register reg;
        unsigned scale;
    };

    struct AddrComponents {
        ssa::Operand &base;
        LargeInt const_offset;
        std::optional<RegOffset> reg_offset;
    };

    typedef std::unordered_map<ssa::BasicBlockIter, mcode::BasicBlockIter> BlockMap;

    target::Target *target;
    std::unordered_map<ssa::VirtualRegister, mcode::StackSlotID> stack_regs;
    std::unordered_map<ssa::VirtualRegister, int> reg_use_counts;
    InstrContext instr_ctx;

    ssa::FunctionDecl *memcpy_func;
    ssa::FunctionDecl *sqrt_func;

protected:
    ssa::Module *ssa_mod;
    ssa::Function *ssa_func;
    ssa::BasicBlockIter ssa_block;
    ssa::InstrIter ssa_instr;

    mcode::Module m_module;
    BlockMap block_map;

public:
    SSALowerer(target::Target *target);
    virtual ~SSALowerer() = default;

    mcode::Module lower_module(ssa::Module &mod);

    const target::Target *get_target() const { return target; }

    ssa::Module &get_module() { return *ssa_mod; }
    ssa::Function &get_func() { return *ssa_func; }
    ssa::BasicBlockIter get_basic_block_iter() { return ssa_block; }
    ssa::BasicBlock &get_block() { return *ssa_block; }
    ssa::InstrIter &get_instr_iter() { return ssa_instr; }
    mcode::Module &get_machine_module() { return m_module; }
    mcode::Function *get_machine_func() { return instr_ctx.func; }

    mcode::InstrIter emit(mcode::Instruction instr);
    mcode::BasicBlockIter split_block();
    void switch_block(mcode::BasicBlockIter block);

    std::optional<mcode::StackSlotID> find_stack_slot(ssa::VirtualRegister reg);
    std::variant<mcode::Register, mcode::StackSlotID> map_vreg(ssa::VirtualRegister reg);
    mcode::Register map_vreg_as_reg(ssa::VirtualRegister reg);
    mcode::Operand map_vreg_as_operand(ssa::VirtualRegister reg, unsigned size);
    mcode::Operand map_vreg_dst(ssa::Instruction &instr, unsigned size);

    unsigned get_size(const ssa::Type &type);
    unsigned get_alignment(const ssa::Type &type);
    unsigned get_member_offset(ssa::Structure *struct_, unsigned index);
    mcode::Register create_tmp_reg();

    ssa::InstrIter get_producer(ssa::VirtualRegister reg);
    ssa::InstrIter get_producer_globally(ssa::VirtualRegister reg);
    unsigned get_num_uses(ssa::VirtualRegister reg);
    void discard_use(ssa::VirtualRegister reg);
    AddrComponents collect_addr(ssa::Operand &addr);

    virtual mcode::CallingConvention *get_calling_convention(ssa::CallingConv calling_conv) = 0;

protected:
    void lower_func(ssa::Function &func);
    mcode::Parameter lower_param(ssa::Type type, mcode::ArgStorage storage, mcode::Function &m_func);
    void create_block(ssa::BasicBlockIter ssa_block);
    void generate_block(ssa::BasicBlockIter ssa_block, mcode::BasicBlockIter m_block);
    void store_graphs();
    void lower_instr(ssa::Instruction &instr);
    void lower_global(ssa::Global &global);

    void lower_alloca(ssa::Instruction &instr);

    virtual void init_module(ssa::Module &mod) {}
    virtual void init_func(ssa::Function &func) {}
    virtual void generate_blocks(ssa::Function &func);
    virtual void emit_block_prologue(ssa::BasicBlock &block) {}

    virtual void lower_load(ssa::Instruction &instr) = 0;
    virtual void lower_store(ssa::Instruction &instr) = 0;
    virtual void lower_loadarg(ssa::Instruction &instr) = 0;
    virtual void lower_add(ssa::Instruction &instr) = 0;
    virtual void lower_sub(ssa::Instruction &instr) = 0;
    virtual void lower_mul(ssa::Instruction &instr) = 0;
    virtual void lower_sdiv(ssa::Instruction &instr) = 0;
    virtual void lower_srem(ssa::Instruction &instr) = 0;
    virtual void lower_udiv(ssa::Instruction &instr) = 0;
    virtual void lower_urem(ssa::Instruction &instr) = 0;
    virtual void lower_fadd(ssa::Instruction &instr) = 0;
    virtual void lower_fsub(ssa::Instruction &instr) = 0;
    virtual void lower_fmul(ssa::Instruction &instr) = 0;
    virtual void lower_fdiv(ssa::Instruction &instr) = 0;
    virtual void lower_and(ssa::Instruction &instr) = 0;
    virtual void lower_or(ssa::Instruction &instr) = 0;
    virtual void lower_xor(ssa::Instruction &instr) = 0;
    virtual void lower_lshl(ssa::Instruction &instr) = 0;
    virtual void lower_lshr(ssa::Instruction &instr) = 0;
    virtual void lower_ashr(ssa::Instruction &instr) = 0;
    virtual void lower_jmp(ssa::Instruction &instr) = 0;
    virtual void lower_cjmp(ssa::Instruction &instr) = 0;
    virtual void lower_fcjmp(ssa::Instruction &instr) = 0;
    virtual void lower_select(ssa::Instruction &instr) = 0;
    virtual void lower_call(ssa::Instruction &instr) = 0;
    virtual void lower_ret(ssa::Instruction &instr) = 0;
    virtual void lower_uextend(ssa::Instruction &instr) = 0;
    virtual void lower_sextend(ssa::Instruction &instr) = 0;
    virtual void lower_truncate(ssa::Instruction &instr) = 0;
    virtual void lower_fpromote(ssa::Instruction &instr) = 0;
    virtual void lower_fdemote(ssa::Instruction &instr) = 0;
    virtual void lower_utof(ssa::Instruction &instr) = 0;
    virtual void lower_stof(ssa::Instruction &instr) = 0;
    virtual void lower_ftou(ssa::Instruction &instr) = 0;
    virtual void lower_ftos(ssa::Instruction &instr) = 0;
    virtual void lower_atomic_load(ssa::Instruction &instr) = 0;
    virtual void lower_atomic_store(ssa::Instruction &instr) = 0;
    virtual void lower_atomic_add(ssa::Instruction &instr) = 0;
    virtual void lower_atomic_sub(ssa::Instruction &instr) = 0;
    virtual void lower_atomic_and(ssa::Instruction &instr) = 0;
    virtual void lower_atomic_or(ssa::Instruction &instr) = 0;
    virtual void lower_atomic_xor(ssa::Instruction &instr) = 0;
    virtual void lower_offsetptr(ssa::Instruction &instr) = 0;
    virtual void lower_memberptr(ssa::Instruction &instr) = 0;
    virtual void lower_copy(ssa::Instruction &instr);
    virtual void lower_sqrt(ssa::Instruction &instr);
    virtual void lower_frame_address(ssa::Instruction &instr) = 0;
};

} // namespace banjo::codegen

#endif
