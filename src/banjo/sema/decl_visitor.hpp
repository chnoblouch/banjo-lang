#ifndef BANJO_SEMA_DECL_VISITOR_H
#define BANJO_SEMA_DECL_VISITOR_H

#include "banjo/sema/semantic_analyzer.hpp"
#include "banjo/sir/sir.hpp"

namespace banjo {

namespace lang {

namespace sema {

class DeclVisitor {

protected:
    SemanticAnalyzer &analyzer;

    DeclVisitor(SemanticAnalyzer &analyzer);

public:
    void analyze(const std::vector<sir::Module *> &mods);
    void analyze_decl_block(sir::DeclBlock &decl_block);
    Result analyze_decl(sir::Decl &decl);
    void visit_symbol(sir::Symbol symbol);

    void visit_func_def(sir::FuncDef &func_def);
    void visit_func_decl(sir::FuncDecl &func_decl);
    void visit_native_func_decl(sir::NativeFuncDecl &native_func_decl);
    Result visit_const_def(sir::ConstDef &const_def);
    void visit_struct_def(sir::StructDef &struct_def);
    void visit_var_decl(sir::VarDecl &var_decl, sir::Decl &out_decl);
    void visit_native_var_decl(sir::NativeVarDecl &native_var_decl);
    void visit_enum_def(sir::EnumDef &enum_def);
    void visit_enum_variant(sir::EnumVariant &enum_variant);
    void visit_union_def(sir::UnionDef &union_def);
    void visit_union_case(sir::UnionCase &union_case);
    void visit_proto_def(sir::ProtoDef &proto_def);
    void visit_type_alias(sir::TypeAlias &type_alias);
    void visit_closure_def(sir::FuncDef &func_def, ClosureContext &closure_ctx);

private:
    virtual Result analyze_func_def(sir::FuncDef & /*func_def*/) { return Result::SUCCESS; }
    virtual Result analyze_func_decl(sir::FuncDecl & /*func_decl*/) { return Result::SUCCESS; }
    virtual Result analyze_native_func_decl(sir::NativeFuncDecl & /*native_func_decl*/) { return Result::SUCCESS; }
    virtual Result analyze_const_def(sir::ConstDef & /*const_def*/) { return Result::SUCCESS; }
    virtual Result analyze_struct_def(sir::StructDef & /*struct_def*/) { return Result::SUCCESS; }
    virtual Result analyze_var_decl(sir::VarDecl & /*var_decl*/, sir::Decl & /*out_decl*/) { return Result::SUCCESS; }
    virtual Result analyze_native_var_decl(sir::NativeVarDecl & /*native_var_decl*/) { return Result::SUCCESS; }
    virtual Result analyze_enum_def(sir::EnumDef & /*enum_def*/) { return Result::SUCCESS; }
    virtual Result analyze_enum_variant(sir::EnumVariant & /*enum_variant*/) { return Result::SUCCESS; }
    virtual Result analyze_union_def(sir::UnionDef & /*union_def*/) { return Result::SUCCESS; }
    virtual Result analyze_union_case(sir::UnionCase & /*union_case*/) { return Result::SUCCESS; }
    virtual Result analyze_proto_def(sir::ProtoDef & /*proto_def*/) { return Result::SUCCESS; }
    virtual Result analyze_type_alias(sir::TypeAlias & /*type_alias*/) { return Result::SUCCESS; }
};

} // namespace sema

} // namespace lang

} // namespace banjo

#endif
