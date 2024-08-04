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
};

} // namespace sema

} // namespace lang

} // namespace banjo

#endif
