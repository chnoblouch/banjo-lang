#ifndef SSA_GENERATOR_H
#define SSA_GENERATOR_H

#include "banjo/ir/module.hpp"
#include "banjo/sir/sir.hpp"
#include "banjo/ssa_gen/ssa_generator_context.hpp"
#include "banjo/target/target.hpp"

namespace banjo {

namespace lang {

class SSAGenerator {

private:
    const sir::Unit &sir_unit;
    ssa::Module ssa_mod;
    SSAGeneratorContext ctx;

public:
    SSAGenerator(const sir::Unit &sir_unit, target::Target *target);
    ssa::Module generate();

private:
    void create_func_def(const sir::FuncDef &sir_func);
    void create_native_func_decl(const sir::NativeFuncDecl &sir_func);
    std::vector<ssa::Type> generate_params(const sir::FuncType &sir_func_type);
    ssa::Type generate_return_type(const sir::Expr &sir_return_type);
    void create_struct_def(const sir::StructDef &sir_struct);

    void generate_func_def(const sir::FuncDef &sir_func);
    void generate_struct_def(const sir::StructDef &sir_struct_def);

    void generate_block(const sir::Block &sir_block);
    void generate_var_stmt(const sir::VarStmt &var_stmt);
    void generate_assign_stmt(const sir::AssignStmt &assign_stmt);
    void generate_return_stmt(const sir::ReturnStmt &return_stmt);
    void generate_if_stmt(const sir::IfStmt &if_stmt);
    void generate_loop_stmt(const sir::LoopStmt &loop_stmt);
    void generate_continue_stmt(const sir::ContinueStmt &continue_stmt);
    void generate_break_stmt(const sir::BreakStmt &break_stmt);
};

} // namespace lang

} // namespace banjo

#endif
