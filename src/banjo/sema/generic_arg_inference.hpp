#ifndef GENERIC_ARG_INFERENCE_H
#define GENERIC_ARG_INFERENCE_H

#include "banjo/sema/semantic_analyzer.hpp"
#include "banjo/sir/sir.hpp"

#include <vector>

namespace banjo {

namespace lang {

namespace sema {

class GenericArgInference {

private:
    SemanticAnalyzer &analyzer;
    const sir::Expr &expr;
    const std::vector<sir::GenericParam> &generic_params;
    const std::vector<sir::Param> &params;

    std::vector<sir::Expr> generic_args;
    std::vector<sir::Expr> inference_sources;
    const sir::Expr *cur_arg;

public:
    GenericArgInference(
        SemanticAnalyzer &analyzer,
        const sir::Expr &expr,
        const std::vector<sir::GenericParam> &generic_params,
        const std::vector<sir::Param> &params
    );

    GenericArgInference(SemanticAnalyzer &analyzer, const sir::Expr &expr, const sir::FuncDef &func_def);

    Result infer(const std::vector<sir::Expr> &args, std::vector<sir::Expr> &out_generic_args);

private:
    Result infer(const sir::Expr &param_type, const sir::Expr &arg_type);
    Result infer_on_ident(const sir::IdentExpr &ident_expr, const sir::Expr &arg_type);
    Result infer_on_pointer_type(const sir::StarExpr &star_expr, const sir::Expr &arg_type);
    Result infer_on_reference_type(const sir::ReferenceType &reference_type, const sir::Expr &arg_type);
};

} // namespace sema

} // namespace lang

} // namespace banjo

#endif
