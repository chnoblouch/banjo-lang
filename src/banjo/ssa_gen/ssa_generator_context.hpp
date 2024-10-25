#ifndef SSA_GENERATOR_CONTEXT_H
#define SSA_GENERATOR_CONTEXT_H

#include "banjo/ir/basic_block.hpp"
#include "banjo/ir/function.hpp"
#include "banjo/ir/instruction.hpp"
#include "banjo/ir/module.hpp"
#include "banjo/ir/virtual_register.hpp"
#include "banjo/sir/sir.hpp"
#include "banjo/target/target.hpp"

#include <deque>
#include <map>
#include <stack>
#include <unordered_map>

namespace banjo {

namespace lang {

enum class ReturnMethod {
    NO_RETURN_VALUE,
    IN_REGISTER,
    VIA_POINTER_ARG,
};

struct DeferredDeinit {
    const sir::Resource *resource;
    ssa::Value ssa_ptr;
};

const ssa::Type DEINIT_FLAG_TYPE = ssa::Primitive::I8;
const ssa::Value DEINIT_FLAG_TRUE = ssa::Value::from_int_immediate(1, DEINIT_FLAG_TYPE);
const ssa::Value DEINIT_FLAG_FALSE = ssa::Value::from_int_immediate(0, DEINIT_FLAG_TYPE);

class SSAGeneratorContext {

public:
    struct DeclContext {
        const sir::Module *sir_mod = nullptr;
        const sir::StructDef *sir_struct_def = nullptr;
        const std::vector<sir::Expr> *sir_generic_args = nullptr;
    };

    struct FuncContext {
        ssa::Function *ssa_func;
        ssa::BasicBlockIter ssa_block;
        ssa::BasicBlockIter ssa_func_exit;
        ssa::VirtualRegister ssa_return_slot;
        ssa::InstrIter ssa_last_alloca;
        std::vector<ir::VirtualRegister> arg_regs;
        std::map<const sir::Resource *, ssa::VirtualRegister> resource_deinit_flags;
        std::vector<DeferredDeinit> cur_deferred_deinits;
        std::vector<const sir::Block *> sir_scopes;
    };

    struct LoopContext {
        ssa::BasicBlockIter ssa_continue_target;
        ssa::BasicBlockIter ssa_break_target;
    };

    target::Target *target;

    ssa::Module *ssa_mod = nullptr;
    std::deque<DeclContext> decl_contexts;
    std::stack<FuncContext> func_contexts;
    std::stack<LoopContext> loop_contexts;

    std::unordered_map<const lang::sir::FuncDef *, ssa::Function *> ssa_funcs;
    std::unordered_map<const lang::sir::Local *, ssa::VirtualRegister> ssa_local_regs;
    std::unordered_map<const lang::sir::Param *, ssa::VirtualRegister> ssa_param_slots;
    std::unordered_map<const void *, ssa::Structure *> ssa_structs;
    std::unordered_map<const lang::sir::VarDecl *, unsigned> ssa_globals;
    std::unordered_map<const lang::sir::NativeVarDecl *, unsigned> ssa_extern_globals;
    std::vector<ssa::Structure *> tuple_structs;

    int string_name_id = 0;
    int block_id = 0;

    SSAGeneratorContext(target::Target *target);

    const std::deque<DeclContext> &get_decl_contexts() { return decl_contexts; }
    DeclContext &push_decl_context();
    void pop_decl_context() { decl_contexts.pop_front(); }

    FuncContext &get_func_context() { return func_contexts.top(); }
    void push_func_context(ssa::Function *ssa_func);
    void pop_func_context() { func_contexts.pop(); }

    LoopContext &get_loop_context() { return loop_contexts.top(); }
    void push_loop_context(LoopContext ctx) { loop_contexts.push(ctx); }
    void pop_loop_context() { loop_contexts.pop(); }

    ssa::Function *get_ssa_func() { return get_func_context().ssa_func; }
    ssa::BasicBlockIter get_ssa_block() { return get_func_context().ssa_block; }

    ir::VirtualRegister next_vreg() { return get_ssa_func()->next_virtual_reg(); }
    std::string next_string_name() { return "str." + std::to_string(string_name_id++); }
    int next_block_id() { return block_id++; }

    ir::BasicBlockIter create_block(std::string label);
    ir::BasicBlockIter create_block();
    void append_block(ir::BasicBlockIter block);

    ir::Instruction &append_alloca(ir::VirtualRegister dst, ir::Type type);
    ir::VirtualRegister append_alloca(ir::Type type);
    ir::Instruction &append_store(ir::Operand src, ir::Operand dst);
    ir::Instruction &append_store(ir::Operand src, ir::VirtualRegister dst);
    ir::Value append_load(ir::Type type, ir::Operand src);
    ir::Value append_load(ir::Type type, ir::VirtualRegister src);
    ir::Instruction &append_loadarg(ir::VirtualRegister dst, ir::Type type, unsigned index);
    ir::VirtualRegister append_loadarg(ir::Type type, unsigned index);
    void append_jmp(ir::BasicBlockIter block_iter);

    void append_cjmp(
        ir::Operand lhs,
        ir::Comparison comparison,
        ir::Operand rhs,
        ir::BasicBlockIter true_block_iter,
        ir::BasicBlockIter false_block_iter
    );

    void append_fcjmp(
        ir::Operand lhs,
        ir::Comparison comparison,
        ir::Operand rhs,
        ir::BasicBlockIter true_block_iter,
        ir::BasicBlockIter false_block_iter
    );

    ir::VirtualRegister append_offsetptr(ir::Operand base, unsigned offset, ir::Type type);
    ir::VirtualRegister append_offsetptr(ir::Operand base, ir::Operand offset, ir::Type type);
    void append_memberptr(ir::VirtualRegister dst, ir::Type type, ir::Operand base, unsigned member);
    ir::VirtualRegister append_memberptr(ir::Type type, ir::Operand base, unsigned member);
    void append_memberptr(ir::VirtualRegister dst, ir::Type type, ir::Operand base, ir::Operand member);
    ir::VirtualRegister append_memberptr(ir::Type type, ir::Operand base, ir::Operand member);
    void append_ret(ir::Operand val);
    void append_ret();
    void append_copy(ir::Operand dst, ir::Operand src, ir::Type type);

    ReturnMethod get_return_method(const ssa::Type return_type);
    ssa::Structure *create_struct(const sir::StructDef &sir_struct_def);
    ssa::Structure *create_union(const sir::UnionDef &sir_union_def);
    ssa::Structure *create_union_case(const sir::UnionCase &sir_union_case);
    ssa::Structure *get_tuple_struct(const std::vector<ssa::Type> &member_types);
};

} // namespace lang

} // namespace banjo

#endif
