#ifndef GENERICS_SPECIALIZER_H
#define GENERICS_SPECIALIZER_H

#include "banjo/sema2/semantic_analyzer.hpp"
#include "banjo/sir/sir.hpp"

namespace banjo {

namespace lang {

namespace sema {

class GenericsSpecializer {

private:
    SemanticAnalyzer &analyzer;

public:
    GenericsSpecializer(SemanticAnalyzer &analyzer);
    sir::FuncDef *specialize(sir::FuncDef &generic_func_def, const std::vector<sir::Expr> &args);
};

} // namespace sema

} // namespace lang

} // namespace banjo

#endif
