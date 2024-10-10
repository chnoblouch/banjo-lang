    #ifndef DECL_INTERFACE_ANALYZER_H
#define DECL_INTERFACE_ANALYZER_H

#include "banjo/sema2/decl_visitor.hpp"
#include "banjo/sema2/semantic_analyzer.hpp"
#include "banjo/sir/sir.hpp"

namespace banjo {

namespace lang {

namespace sema {

class DeclInterfaceAnalyzer final : public DeclVisitor {

public:
    DeclInterfaceAnalyzer(SemanticAnalyzer &analyzer);

    Result analyze_func_def(sir::FuncDef &func_def) override;
    Result analyze_native_func_decl(sir::NativeFuncDecl &native_func_decl) override;
    Result analyze_const_def(sir::ConstDef &const_def) override;
    Result analyze_var_decl(sir::VarDecl &var_decl, sir::Decl &out_decl) override;
    Result analyze_native_var_decl(sir::NativeVarDecl &native_var_decl) override;
    Result analyze_enum_variant(sir::EnumVariant &enum_variant) override;
    Result analyze_union_case(sir::UnionCase &union_case) override;
};

} // namespace sema

} // namespace lang

} // namespace banjo

#endif
