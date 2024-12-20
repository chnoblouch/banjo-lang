#ifndef DECL_VISITOR_H
#define DECL_VISITOR_H

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
    void analyze_meta_block(sir::MetaBlock &meta_block);
    Result analyze_decl(sir::Decl &decl);

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
    virtual Result analyze_use_decl(sir::UseDecl & /*use_decl*/) { return Result::SUCCESS; }

    Result process_func_def(sir::FuncDef &func_def);
    void process_struct_def(sir::StructDef &struct_def);
    void process_enum_def(sir::EnumDef &enum_def);
    void process_union_def(sir::UnionDef &union_def);
    void process_proto_def(sir::ProtoDef &proto_def);
};

} // namespace sema

} // namespace lang

} // namespace banjo

#endif
