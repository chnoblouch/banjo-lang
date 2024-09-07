#include "generic_arg_inference.hpp"

#include "banjo/utils/macros.hpp"

#include <utility>

namespace banjo {

namespace lang {

namespace sema {

GenericArgInference::GenericArgInference(
    SemanticAnalyzer &analyzer,
    const sir::Expr &expr,
    const std::vector<sir::GenericParam> &generic_params,
    const std::vector<sir::Param> &params
)
  : analyzer(analyzer),
    expr(expr),
    generic_params(generic_params),
    params(params),
    generic_args(generic_params.size(), nullptr),
    inference_sources(generic_params.size(), nullptr) {}

GenericArgInference::GenericArgInference(
    SemanticAnalyzer &analyzer,
    const sir::Expr &expr,
    const sir::FuncDef &func_def
)
  : GenericArgInference(analyzer, expr, func_def.generic_params, func_def.type.params) {
    ASSUME(func_def.is_generic());
}

Result GenericArgInference::infer(const std::vector<sir::Expr> &args, std::vector<sir::Expr> &out_generic_args) {
    Result result = Result::SUCCESS;
    Result partial_result;

    for (unsigned i = 0; i < args.size(); i++) {
        ASSUME(args[i].get_type());

        cur_arg = &args[i];
        partial_result = infer(params[i].type, cur_arg->get_type());

        if (partial_result != Result::SUCCESS) {
            result = partial_result;
        }
    }

    for (unsigned i = 0; i < generic_args.size(); i++) {
        if (!generic_args[i]) {
            analyzer.report_generator.report_err_cannot_infer_generic_arg(expr, generic_params[i]);
            result = Result::ERROR;
        }
    }

    out_generic_args = std::move(generic_args);
    return result;
}

Result GenericArgInference::infer(const sir::Expr &param_type, const sir::Expr &arg_type) {
    if (auto ident_expr = param_type.match<sir::IdentExpr>()) {
        return infer_on_ident(*ident_expr, arg_type);
    } else if (auto star_expr = param_type.match<sir::StarExpr>()) {
        return infer_on_pointer_type(*star_expr, arg_type);
    } else {
        return Result::SUCCESS;
    }
}

Result GenericArgInference::infer_on_ident(const sir::IdentExpr &ident_expr, const sir::Expr &arg_type) {
    for (unsigned i = 0; i < generic_params.size(); i++) {
        const sir::GenericParam &generic_param = generic_params[i];

        if (generic_param.ident.value != ident_expr.value) {
            continue;
        }

        if (generic_args[i]) {
            analyzer.report_generator
                .report_err_generic_arg_inference_conflict(expr, generic_param, inference_sources[i], *cur_arg);
            return Result::ERROR;
        }

        generic_args[i] = arg_type;
        inference_sources[i] = *cur_arg;
    }

    return Result::SUCCESS;
}

Result GenericArgInference::infer_on_pointer_type(const sir::StarExpr &star_expr, const sir::Expr &arg_type) {
    if (auto arg_pointer_type = arg_type.match<sir::PointerType>()) {
        return infer(star_expr.value, arg_pointer_type->base_type);
    } else {
        return Result::SUCCESS;
    }
}

} // namespace sema

} // namespace lang

} // namespace banjo
