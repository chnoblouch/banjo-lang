#ifndef BANJO_SEMA_OVERLOAD_RESOLVER_H
#define BANJO_SEMA_OVERLOAD_RESOLVER_H

#include "banjo/sema/semantic_analyzer.hpp"
#include "banjo/sir/sir.hpp"

namespace banjo {
namespace lang {
namespace sema {

class OverloadResolver {

private:
    SemanticAnalyzer &analyzer;

public:
    OverloadResolver(SemanticAnalyzer &analyzer);
    sir::FuncDef *resolve(sir::OverloadSet &set, const std::vector<sir::Expr> &args);
    bool is_matching_overload(sir::FuncDef &func_def, const std::vector<sir::Expr> &args);

private:
    bool is_coercible(sir::Expr arg_type, sir::Expr param_type);
};

} // namespace sema
} // namespace lang
} // namespace banjo

#endif
