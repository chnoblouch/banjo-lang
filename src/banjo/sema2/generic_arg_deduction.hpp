#ifndef GENERIC_ARG_DEDUCTION_H
#define GENERIC_ARG_DEDUCTION_H

#include "banjo/sema2/semantic_analyzer.hpp"
#include "banjo/sir/sir.hpp"

#include <optional>
#include <vector>

namespace banjo {

namespace lang {

namespace sema {

class GenericArgDeduction {

private:
    SemanticAnalyzer &analyzer;

public:
    GenericArgDeduction(SemanticAnalyzer &analyzer);

    std::optional<std::vector<sir::Expr>> deduce(
        const std::vector<sir::GenericParam> &generic_params,
        const std::vector<sir::Expr> &args
    );
};

} // namespace sema

} // namespace lang

} // namespace banjo

#endif
