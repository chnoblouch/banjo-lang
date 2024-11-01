#ifndef DECL_BODY_ANALYZER_H
#define DECL_BODY_ANALYZER_H

#include "banjo/sema2/decl_visitor.hpp"
#include "banjo/sema2/semantic_analyzer.hpp"
#include "banjo/sir/sir.hpp"

namespace banjo {

namespace lang {

namespace sema {

class DeclBodyAnalyzer final : public DeclVisitor {

public:
    DeclBodyAnalyzer(SemanticAnalyzer &analyzer);
    Result analyze_func_def(sir::FuncDef &func_def) override;
    Result analyze_const_def(sir::ConstDef &const_def) override;
    Result analyze_struct_def(sir::StructDef &struct_def) override;
    Result analyze_var_decl(sir::VarDecl &var_decl, sir::Decl &out_decl) override;
    Result analyze_enum_def(sir::EnumDef &enum_def) override;

private:
    void analyze_proto_impl(sir::StructDef &struct_def, sir::ProtoDef &proto_def);
};

} // namespace sema

} // namespace lang

} // namespace banjo

#endif
