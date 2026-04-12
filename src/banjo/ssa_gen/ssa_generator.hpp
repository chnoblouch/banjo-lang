#ifndef BANJO_SSA_GENERATOR_H
#define BANJO_SSA_GENERATOR_H

#include "banjo/sir/sir.hpp"
#include "banjo/ssa/module.hpp"
#include "banjo/ssa/structure.hpp"
#include "banjo/ssa_gen/specialization_collector.hpp"
#include "banjo/ssa_gen/ssa_generator_context.hpp"
#include "banjo/target/target.hpp"

#include <vector>

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
    void create_func_defs(const sir::FuncDef &sir_func);
    ssa::Function *create_func_def(const sir::FuncDef &sir_func, const std::vector<sir::Expr> &sir_generic_args);
    void create_native_func_decl(const sir::NativeFuncDecl &sir_func);
    std::vector<ssa::Type> generate_params(const sir::FuncType &sir_func_type);
    ssa::Type generate_return_type(const sir::Expr &sir_return_type);
    void create_struct_defs(const sir::StructDef &sir_struct);
    ssa::Structure *create_struct_def(const sir::StructDef &sir_struct, const std::vector<sir::Expr> &sir_generic_args);
    void create_union_def(const sir::UnionDef &sir_union_def);
    void create_proto_def(const sir::ProtoDef &sir_proto_def);
    void create_var_decl(const sir::VarDecl &sir_var_decl);
    void create_native_var_decl(const sir::NativeVarDecl &sir_native_var_decl);

    void generate_runtime();

    void generate_types(const sir::DeclBlock &decl_block);
    void generate_struct_def_types(const sir::StructDef &sir_struct);
    void generate_struct_def_type(const sir::StructDef &sir_struct, ssa::Structure &ssa_struct);

    void generate_decls(const sir::DeclBlock &decl_block);
    void generate_func_defs(const sir::FuncDef &sir_func);
    void generate_func_def(const sir::FuncDef &sir_func, ssa::Function &ssa_func);
    void generate_struct_defs(const sir::StructDef &sir_struct);
    void generate_struct_def(const sir::StructDef &sir_struct);
    void generate_union_def(const sir::UnionDef &sir_union_def);
    void generate_proto_def(const sir::ProtoDef &sir_proto_def);
    void generate_var_decl(const sir::VarDecl &sir_var_decl);
    void generate_native_var_decl(const sir::NativeVarDecl &sir_native_var_decl);

    void push_specialization(SpecializationCollector::Entry &specialization);
    void pop_specialization(SpecializationCollector::Entry &specialization);
};

} // namespace lang

} // namespace banjo

#endif
