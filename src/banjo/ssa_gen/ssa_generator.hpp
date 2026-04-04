#ifndef BANJO_SSA_GENERATOR_H
#define BANJO_SSA_GENERATOR_H

#include "banjo/sir/sir.hpp"
#include "banjo/ssa/module.hpp"
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
    void create_struct_def(const sir::StructDef &sir_struct_def);
    void create_union_def(const sir::UnionDef &sir_union_def);
    void create_proto_def(const sir::ProtoDef &sir_proto_def);
    void create_var_decl(const sir::VarDecl &sir_var_decl);
    void create_native_var_decl(const sir::NativeVarDecl &sir_native_var_decl);

    void generate_runtime();

    void generate_decls(const sir::DeclBlock &decl_block);
    void generate_func_def(const sir::FuncDef &sir_func);
    void generate_func_body(const sir::FuncDef &sir_func, ssa::Function &ssa_func);
    void generate_struct_def(const sir::StructDef &sir_struct_def);
    void generate_union_def(const sir::UnionDef &sir_union_def);
    void generate_proto_def(const sir::ProtoDef &sir_proto_def);
    void generate_var_decl(const sir::VarDecl &sir_var_decl);
    void generate_native_var_decl(const sir::NativeVarDecl &sir_native_var_decl);

    void insert_generic_args(const SpecializationCollector::Entry &specialization);
    void remove_generic_args(const SpecializationCollector::Entry &specialization);

    template <typename T>
    void insert_generic_args(const sir::Specialization<T> *specialization) {
        if (!specialization) {
            return;
        }

        const T &generic_def = *specialization->generic_def;

        for (unsigned i = 0; i < specialization->args.size(); i++) {
            sir::Expr arg = specialization->args[i];
            ctx.sir_generic_args.emplace(&generic_def.generic_params[i], arg);
        }
    }

    template <typename T>
    void remove_generic_args(const sir::Specialization<T> *specialization) {
        if (!specialization) {
            return;
        }

        const T &generic_def = *specialization->generic_def;

        for (const sir::GenericParam &generic_param : generic_def.generic_params) {
            ctx.sir_generic_args.erase(&generic_param);
        }
    }
};

} // namespace lang

} // namespace banjo

#endif
