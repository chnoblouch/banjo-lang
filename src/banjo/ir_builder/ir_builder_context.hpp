#ifndef IR_BUILDER_IR_BUILDER_CONTEXT_H
#define IR_BUILDER_IR_BUILDER_CONTEXT_H

#include "ir/module.hpp"
#include "ir/structure.hpp"
#include "ir/virtual_register.hpp"
#include "symbol/data_type.hpp"
#include "symbol/function.hpp"
#include "symbol/module_path.hpp"
#include "symbol/structure.hpp"
#include "target/target.hpp"

#include <stack>
#include <string>
#include <unordered_map>
#include <vector>

namespace banjo {

namespace lang {
class Variable;
} // namespace lang

namespace ir_builder {

struct Closure {
    lang::ASTNode *node;
    ir::Type context_type;
    std::vector<lang::Variable *> captured_vars;
};

struct Scope {
    ir::BasicBlockIter loop_exit;
    ir::BasicBlockIter loop_entry;
};

class IRBuilderContext {

private:
    target::Target *target;

    ir::Module *current_mod = nullptr;
    lang::ModulePath current_mod_path;
    ir::Function *current_func = nullptr;
    ir::BasicBlockIter cur_block_iter;
    ir::Structure *current_struct = nullptr;
    lang::Function *current_lang_func = nullptr;
    lang::Structure *current_lang_struct = nullptr;
    Closure *current_closure = nullptr;

    std::vector<ir::VirtualRegister> current_arg_regs;
    ir::VirtualRegister current_return_reg{-1};
    ir::BasicBlockIter cur_func_exit;
    ir::InstrIter cur_last_alloca;
    std::stack<Scope> scopes;

    std::unordered_map<std::string, ir::Type> generic_types;
    std::vector<ir::Structure*> tuple_structs;

    int string_name_id = 0;
    int block_id = 0;
    int if_chain_id = 0;
    int switch_id = 0;
    int try_id = 0;
    int while_id = 0;
    int for_id = 0;
    int or_id = 0;
    int and_id = 0;
    int not_id = 0;
    int cmp_to_val_id = 0;
    int deinit_flag_id = 0;

public:
    int closure_id = 0;
    bool has_branch;

    IRBuilderContext(target::Target *target);
    target::Target *get_target() { return target; }

    ir::Module *get_current_mod() { return current_mod; }
    lang::ModulePath &get_current_mod_path() { return current_mod_path; }
    ir::Function *get_current_func() { return current_func; }
    ir::BasicBlockIter get_cur_block_iter() { return cur_block_iter; }
    ir::BasicBlock &get_cur_block() { return *cur_block_iter; }
    ir::Structure *get_current_struct() { return current_struct; }
    lang::Function *get_current_lang_func() { return current_lang_func; }
    lang::Structure *get_current_lang_struct() { return current_lang_struct; }
    Closure *get_current_closure() { return current_closure; }
    std::vector<ir::VirtualRegister> &get_current_arg_regs() { return current_arg_regs; }
    ir::VirtualRegister get_current_return_reg() { return current_return_reg; }
    ir::BasicBlockIter get_cur_func_exit() { return cur_func_exit; }
    Scope &get_scope() { return scopes.top(); }

    void set_current_mod(ir::Module *current_mod) { this->current_mod = current_mod; }
    void set_current_mod_path(lang::ModulePath current_mod_path) { this->current_mod_path = current_mod_path; }
    void set_current_func(ir::Function *current_func);
    void set_cur_block_iter(ir::BasicBlockIter cur_block_iter) { this->cur_block_iter = cur_block_iter; }
    void set_current_struct(ir::Structure *current_struct) { this->current_struct = current_struct; }
    void set_current_lang_func(lang::Function *current_lang_func) { this->current_lang_func = current_lang_func; }
    void set_current_lang_struct(lang::Structure *current_lang_struct) {
        this->current_lang_struct = current_lang_struct;
    }
    void set_current_closure(Closure *current_closure) { this->current_closure = current_closure; }
    void set_current_arg_regs(std::vector<ir::VirtualRegister> current_arg_regs) {
        this->current_arg_regs = current_arg_regs;
    }
    void set_current_return_reg(ir::VirtualRegister current_return_reg) {
        this->current_return_reg = current_return_reg;
    }
    void set_cur_func_exit(ir::BasicBlockIter cur_func_exit) { this->cur_func_exit = cur_func_exit; }
    void set_generic_types(std::unordered_map<std::string, ir::Type> generic_types) {
        this->generic_types = generic_types;
    }

    std::unordered_map<std::string, ir::Type> &get_generic_types() { return generic_types; }
    Scope &push_scope();
    void pop_scope();

    std::string next_string_name();
    int next_block_id();
    int next_if_chain_id();
    int next_switch_id();
    int next_try_id() { return try_id++; }
    int next_while_id();
    int next_for_id();
    int next_or_id();
    int next_and_id();
    int next_not_id();
    int next_cmp_to_val_id();
    int next_deinit_flag_id();

    unsigned get_size(const ir::Type &type);

    ir::BasicBlockIter create_block(std::string label);
    void append_block(ir::BasicBlockIter block);
    ir::Instruction &append_alloca(ir::VirtualRegister dest, ir::Type type);
    ir::VirtualRegister append_alloca(ir::Type type);
    ir::Instruction &append_store(ir::Operand src, ir::Operand dest);
    void append_load(ir::VirtualRegister dest, ir::Type type, ir::Operand src);
    ir::Value append_load(ir::Type type, ir::Operand src);
    ir::Instruction &append_loadarg(ir::VirtualRegister dst, ir::Type type, unsigned index);
    ir::VirtualRegister append_loadarg(ir::Type type, unsigned index);
    void append_jmp(ir::BasicBlockIter block_iter);

    void append_cjmp(
        ir::Operand left,
        ir::Comparison comparison,
        ir::Operand right,
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

    ir::Type build_type(lang::DataType *type);
    ir::Structure *get_tuple_struct(const std::vector<ir::Type> &member_types);
};

} // namespace ir_builder

} // namespace banjo

#endif
