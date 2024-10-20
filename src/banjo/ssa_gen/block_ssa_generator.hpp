#ifndef BLOCK_SSA_GENERATOR_H
#define BLOCK_SSA_GENERATOR_H

#include "banjo/sir/sir.hpp"
#include "banjo/ssa_gen/ssa_generator_context.hpp"

namespace banjo {

namespace lang {

class BlockSSAGenerator {

private:
    SSAGeneratorContext &ctx;

public:
    BlockSSAGenerator(SSAGeneratorContext &ctx);

    void generate_block(const sir::Block &block);
    void generate_block_allocas(const sir::Block &block);
    void generate_resource_flags(const sir::Resource &resource);
    void generate_block_body(const sir::Block &block);
    void generate_block_deinit(const sir::Block &block);

private:
    void generate_var_stmt(const sir::VarStmt &var_stmt);
    void generate_assign_stmt(const sir::AssignStmt &assign_stmt);
    void generate_return_stmt(const sir::ReturnStmt &return_stmt);
    void generate_if_stmt(const sir::IfStmt &if_stmt);
    void generate_switch_stmt(const sir::SwitchStmt &switch_stmt);
    void generate_loop_stmt(const sir::LoopStmt &loop_stmt);
    void generate_continue_stmt(const sir::ContinueStmt &continue_stmt);
    void generate_break_stmt(const sir::BreakStmt &break_stmt);

    void generate_deinit(const sir::Resource &resource, sir::Symbol symbol);
    void generate_deinit(const sir::Resource &resource, ssa::Value ssa_ptr);
    void generate_deinit_call(const sir::Resource &resource, ssa::Value ssa_ptr);
};

} // namespace lang

} // namespace banjo

#endif
