#ifndef BANJO_SEMA_OVERLOAD_RESOLVER_H
#define BANJO_SEMA_OVERLOAD_RESOLVER_H

#include "banjo/sema/semantic_analyzer.hpp"
#include "banjo/sir/sir.hpp"

#include <span>

namespace banjo::sema {

class OverloadResolver {

private:
    SemanticAnalyzer &analyzer;

public:
    OverloadResolver(SemanticAnalyzer &analyzer);
    sir::FuncDef *resolve(sir::OverloadSet &set, std::span<sir::Expr> args, std::span<sir::Expr> generic_args);
    bool is_matching_overload(sir::FuncDef &func_def, std::span<sir::Expr> args, std::span<sir::Expr> generic_args);

private:
    bool is_coercible(sir::Expr arg_type, sir::Expr param_type);
};

} // namespace banjo::sema

#endif
