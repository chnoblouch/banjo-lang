#include "decl_body_analyzer.hpp"

#include "banjo/sema2/stmt_analyzer.hpp"

namespace banjo {

namespace lang {

namespace sema {

DeclBodyAnalyzer::DeclBodyAnalyzer(SemanticAnalyzer &analyzer) : DeclVisitor(analyzer) {}

Result DeclBodyAnalyzer::analyze_func_def(sir::FuncDef &func_def) {
    analyzer.push_scope().func_def = &func_def;
    StmtAnalyzer(analyzer).analyze_block(func_def.block);
    analyzer.pop_scope();

    return Result::SUCCESS;
}

} // namespace sema

} // namespace lang

} // namespace banjo
