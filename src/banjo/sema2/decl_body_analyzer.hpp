#ifndef DECL_BODY_ANALYZER_H
#define DECL_BODY_ANALYZER_H

#include "banjo/sema2/semantic_analyzer.hpp"
#include "banjo/sir/sir.hpp"

namespace banjo {

namespace lang {

namespace sema {

class DeclBodyAnalyzer {

private:
    SemanticAnalyzer analyzer;

public:
    DeclBodyAnalyzer(SemanticAnalyzer &analyzer);

    void analyze();
    void analyze_decl_block(sir::DeclBlock &decl_block);
    void analyze_func_def(sir::FuncDef &func_def);
    void analyze_const_def(sir::ConstDef &const_def);
    void analyze_struct_def(sir::StructDef &struct_def);
    void analyze_enum_def(sir::EnumDef &enum_def);
};

} // namespace sema

} // namespace lang

} // namespace banjo

#endif
