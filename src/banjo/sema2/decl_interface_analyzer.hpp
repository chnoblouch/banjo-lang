#ifndef DECL_INTERFACE_ANALYZER_H
#define DECL_INTERFACE_ANALYZER_H

#include "banjo/sema2/semantic_analyzer.hpp"
#include "banjo/sir/sir.hpp"

namespace banjo {

namespace lang {

namespace sema {

class DeclInterfaceAnalyzer {

private:
    SemanticAnalyzer analyzer;

public:
    DeclInterfaceAnalyzer(SemanticAnalyzer &analyzer);
    
    void analyze();
    void analyze_decl_block(sir::DeclBlock &decl_block);
    void analyze_func_def(sir::FuncDef &func_def);
    void analyze_native_func_decl(sir::NativeFuncDecl &native_func_decl);
    void analyze_struct_def(sir::StructDef &struct_def);
    void analyze_var_decl(sir::VarDecl &var_decl, sir::Decl &out_decl);
};

} // namespace sema

} // namespace lang

} // namespace banjo

#endif
