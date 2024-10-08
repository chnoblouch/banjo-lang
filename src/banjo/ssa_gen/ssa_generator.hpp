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
    void create_decls(const sir::DeclBlock &decl_block);
    void create_func_def(const sir::FuncDef &sir_func);
    void create_native_func_decl(const sir::NativeFuncDecl &sir_func);
    std::vector<ssa::Type> generate_params(const sir::FuncType &sir_func_type);
    ssa::Type generate_return_type(const sir::Expr &sir_return_type);
    void create_struct_def(const sir::StructDef &sir_struct_def, const std::vector<sir::Expr> *generic_args = nullptr);
    void create_union_def(const sir::UnionDef &sir_union_def);
    void create_var_decl(const sir::VarDecl &sir_var_decl);
    void create_native_var_decl(const sir::NativeVarDecl &sir_native_var_decl);

    void generate_decls(const sir::DeclBlock &decl_block);
    void generate_func_def(const sir::FuncDef &sir_func);
    void generate_struct_def(const sir::StructDef &sir_struct_def);
    void generate_union_def(const sir::UnionDef &sir_union_def);
    void generate_var_decl(const sir::VarDecl &sir_var_decl);
    void generate_native_var_decl(const sir::NativeVarDecl &sir_native_var_decl);

    void generate_block(const sir::Block &sir_block);
    void generate_block_allocas(const sir::Block &sir_block);
    void generate_block_stmts(const sir::Block &sir_block);

    void generate_var_stmt(const sir::VarStmt &var_stmt);
    void generate_assign_stmt(const sir::AssignStmt &assign_stmt);
    void generate_return_stmt(const sir::ReturnStmt &return_stmt);
    void generate_if_stmt(const sir::IfStmt &if_stmt);
    void generate_switch_stmt(const sir::SwitchStmt &switch_stmt);
    void generate_loop_stmt(const sir::LoopStmt &loop_stmt);
    void generate_continue_stmt(const sir::ContinueStmt &continue_stmt);
    void generate_break_stmt(const sir::BreakStmt &break_stmt);
};

} // namespace lang

} // namespace banjo

#endif
