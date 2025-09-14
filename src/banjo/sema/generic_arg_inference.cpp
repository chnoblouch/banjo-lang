#include "generic_arg_inference.hpp"

#include "banjo/sir/sir.hpp"
#include "banjo/utils/macros.hpp"

#include <utility>

namespace banjo {

namespace lang {

namespace sema {

GenericArgInference::GenericArgInference(
    SemanticAnalyzer &analyzer,
    const sir::Expr &expr,
    const std::vector<sir::GenericParam> &generic_params,
    std::span<sir::Param> params
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
    ASSERT(func_def.is_generic());
}

Result GenericArgInference::infer(std::span<sir::Expr> args, std::span<sir::Expr> &out_generic_args) {
    Result result = Result::SUCCESS;
    Result partial_result;

    bool has_sequence = generic_params.back().kind == sir::GenericParamKind::SEQUENCE;
    unsigned non_sequence_end = has_sequence ? params.size() - 1 : params.size();
    unsigned num_sequence_args = has_sequence ? args.size() - (params.size() - 1) : 0;

    if (args.size() < non_sequence_end) {
        analyzer.report_generator.report_err_too_few_args_to_infer_generic_args(expr);
        return Result::ERROR;
    }

    for (unsigned i = 0; i < non_sequence_end; i++) {
        if (!args[i].get_type()) {
            continue;
        }

        cur_arg = &args[i];
        partial_result = infer(params[i].type, cur_arg->get_type());

        if (partial_result != Result::SUCCESS) {
            result = partial_result;
        }
    }

    if (has_sequence) {
        std::span<sir::Expr> sequence_types = analyzer.allocate_array<sir::Expr>(num_sequence_args);

        for (unsigned i = 0; i < num_sequence_args; i++) {
            sequence_types[i] = args[non_sequence_end + i].get_type();
        }

        generic_args.back() = analyzer.create(
            sir::TupleExpr{
                .ast_node = nullptr,
                .type = nullptr,
                .exprs = sequence_types,
            }
        );
    }

    for (unsigned i = 0; i < generic_args.size(); i++) {
        if (!generic_args[i]) {
            analyzer.report_generator.report_err_cannot_infer_generic_arg(expr, generic_params[i]);
            result = Result::ERROR;
        }
    }

    out_generic_args = analyzer.allocate_array<sir::Expr>(generic_args.size());
    std::copy(generic_args.begin(), generic_args.end(), out_generic_args.begin());
    return result;
}

Result GenericArgInference::infer(const sir::Expr &param_type, const sir::Expr &arg_type) {
    if (auto ident_expr = param_type.match<sir::IdentExpr>()) {
        return infer_on_ident(*ident_expr, arg_type);
    } else if (auto star_expr = param_type.match<sir::StarExpr>()) {
        return infer_on_pointer_type(*star_expr, arg_type);
    } else if (auto reference_type = param_type.match<sir::ReferenceType>()) {
        return infer_on_reference_type(*reference_type, arg_type);
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

        if (generic_args[i] && generic_args[i] != arg_type) {
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

Result GenericArgInference::infer_on_reference_type(
    const sir::ReferenceType &reference_type,
    const sir::Expr &arg_type
) {
    if (auto arg_reference_type = arg_type.match<sir::ReferenceType>()) {
        // If the argument is already a reference, unwrap both reference types.
        return infer(reference_type.base_type, arg_reference_type->base_type);
    } else {
        // If the argument isn't a reference, unwrap the reference type to infer from the base type.
        return infer(reference_type.base_type, arg_type);
    }
}

} // namespace sema

} // namespace lang

} // namespace banjo
