#ifndef BANJO_SEMA_GENERIC_ARG_INFERENCE_H
#define BANJO_SEMA_GENERIC_ARG_INFERENCE_H

#include "banjo/sema/semantic_analyzer.hpp"
#include "banjo/sir/sir.hpp"

#include <span>
#include <vector>

namespace banjo {

namespace lang {

namespace sema {

class GenericArgInference {

private:
    SemanticAnalyzer &analyzer;
    const sir::Expr &expr;
    std::span<sir::GenericParam *> generic_params;
    std::span<sir::Param> params;

    std::vector<sir::Expr> generic_args;
    std::vector<sir::Expr> inference_sources;
    const sir::Expr *cur_arg;

public:
    GenericArgInference(
        SemanticAnalyzer &analyzer,
        const sir::Expr &expr,
        std::span<sir::GenericParam *> generic_params,
        std::span<sir::Param> params
    );

    GenericArgInference(SemanticAnalyzer &analyzer, const sir::Expr &expr, const sir::FuncDef &func_def);

    Result infer(std::span<sir::Expr> args, std::span<sir::Expr> &out_generic_args);

private:
    Result infer(sir::Expr &param_type, sir::Expr arg_type);
    Result infer_on_symbol_expr(sir::SymbolExpr &symbol_expr, sir::Expr arg_type);
    Result infer_on_pointer_type(sir::PointerType &pointer_type, sir::Expr arg_type);
    Result infer_on_closure_type(sir::ClosureType &closure_type, sir::Expr arg_type);
    Result infer_on_reference_type(sir::ReferenceType &reference_type, sir::Expr arg_type);
};

} // namespace sema

} // namespace lang

} // namespace banjo

#endif
