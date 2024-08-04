#ifndef DECL_VALUE_ANALYZER_H
#define DECL_VALUE_ANALYZER_H

#include "banjo/sema2/decl_visitor.hpp"
#include "banjo/sema2/semantic_analyzer.hpp"
#include "banjo/sir/sir.hpp"

namespace banjo {

namespace lang {

namespace sema {

class DeclValueAnalyzer final : public DeclVisitor {

public:
    DeclValueAnalyzer(SemanticAnalyzer &analyzer);

    Result analyze_const_def(sir::ConstDef &const_def) override;
    Result analyze_enum_def(sir::EnumDef &enum_def) override;
};

} // namespace sema

} // namespace lang

} // namespace banjo

#endif
