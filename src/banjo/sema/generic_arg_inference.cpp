#include "generic_arg_inference.hpp"

#include "banjo/sema/result_macros.hpp"
#include "banjo/sema/semantic_analyzer.hpp"
#include "banjo/sir/sir.hpp"
#include "banjo/utils/macros.hpp"

namespace banjo::sema {

GenericArgInference::GenericArgInference(SemanticAnalyzer &analyzer, sir::CallExpr &call_expr, sir::FuncDef &func_def)
  : analyzer{analyzer},
    call_expr{call_expr},
    func_def{func_def},
    generic_params{func_def.generic_params},
    params{func_def.type.params},
    generic_args(generic_params.size(), nullptr),
    inference_sources(generic_params.size(), nullptr) {
    ASSERT(func_def.is_generic());
}

Result GenericArgInference::infer(std::span<sir::Expr> &out_generic_args) {
    Result result = Result::SUCCESS;
    Result partial_result;

    std::span<sir::Expr> args = call_expr.args;

    bool has_sequence = generic_params.back()->kind == sir::GenericParamKind::SEQUENCE;
    unsigned non_sequence_end = has_sequence ? params.size() - 1 : params.size();
    unsigned num_sequence_args = has_sequence ? args.size() - (params.size() - 1) : 0;

    if (args.size() < non_sequence_end) {
        analyzer.report_generator.report_err_unexpected_arg_count(call_expr, non_sequence_end, &func_def);
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
            analyzer.report_generator.report_err_cannot_infer_generic_arg(&call_expr, *generic_params[i]);
            result = Result::ERROR;
        }
    }

    out_generic_args = analyzer.allocate_array<sir::Expr>(generic_args.size());
    std::copy(generic_args.begin(), generic_args.end(), out_generic_args.begin());
    return result;
}

Result GenericArgInference::infer(sir::Expr &param_type, sir::Expr arg_type) {
    if (auto symbol_expr = param_type.match<sir::SymbolExpr>()) {
        return infer_on_symbol_expr(*symbol_expr, arg_type);
    } else if (auto tuple_expr = param_type.match<sir::TupleExpr>()) {
        return infer_on_tuple_expr(*tuple_expr, arg_type);
    } else if (auto specialize_expr = param_type.match<sir::SpecializeExpr>()) {
        return infer_on_specialize_expr(*specialize_expr, arg_type);
    } else if (auto pointer_type = param_type.match<sir::PointerType>()) {
        return infer_on_pointer_type(*pointer_type, arg_type);
    } else if (auto closure_type = param_type.match<sir::ClosureType>()) {
        return infer_on_closure_type(*closure_type, arg_type);
    } else if (auto reference_type = param_type.match<sir::ReferenceType>()) {
        return infer_on_reference_type(*reference_type, arg_type);
    } else {
        return Result::SUCCESS;
    }
}

Result GenericArgInference::infer_on_symbol_expr(sir::SymbolExpr &symbol_expr, sir::Expr arg_type) {
    sir::GenericParam *generic_param = symbol_expr.symbol.match<sir::GenericParam>();

    if (!generic_param) {
        return Result::SUCCESS;
    }

    for (unsigned i = 0; i < generic_params.size(); i++) {
        if (generic_param != generic_params[i]) {
            continue;
        }

        if (generic_args[i] && generic_args[i] != arg_type) {
            analyzer.report_generator
                .report_err_generic_arg_inference_conflict(&call_expr, *generic_param, inference_sources[i], *cur_arg);
            return Result::ERROR;
        }

        generic_args[i] = arg_type;
        inference_sources[i] = *cur_arg;
    }

    return Result::SUCCESS;
}

Result GenericArgInference::infer_on_tuple_expr(sir::TupleExpr &tuple_expr, sir::Expr arg_type) {
    if (auto arg_tuple_expr = arg_type.match<sir::TupleExpr>()) {
        Result result = Result::SUCCESS;

        for (unsigned i = 0; i < std::min(tuple_expr.exprs.size(), arg_tuple_expr->exprs.size()); i++) {
            RESULT_MERGE(result, infer(tuple_expr.exprs[i], arg_tuple_expr->exprs[i]));
        }

        return result;
    } else {
        return Result::SUCCESS;
    }
}

Result GenericArgInference::infer_on_specialize_expr(sir::SpecializeExpr &specialize_expr, sir::Expr arg_type) {
    if (auto arg_specialize_expr = arg_type.match<sir::SpecializeExpr>()) {
        Result result = Result::SUCCESS;

        for (unsigned i = 0; i < std::min(specialize_expr.args.size(), arg_specialize_expr->args.size()); i++) {
            RESULT_MERGE(result, infer(specialize_expr.args[i], arg_specialize_expr->args[i]));
        }

        return result;
    } else {
        return Result::SUCCESS;
    }
}

Result GenericArgInference::infer_on_pointer_type(sir::PointerType &pointer_type, sir::Expr arg_type) {
    if (auto arg_pointer_type = arg_type.match<sir::PointerType>()) {
        return infer(pointer_type.base_type, arg_pointer_type->base_type);
    } else {
        return Result::SUCCESS;
    }
}

Result GenericArgInference::infer_on_closure_type(sir::ClosureType &closure_type, sir::Expr arg_type) {
    if (auto arg_closure_type = arg_type.match<sir::ClosureType>()) {
        Result result = Result::SUCCESS;
        Result partial_result;

        sir::FuncType &func_type = closure_type.func_type;
        sir::FuncType &arg_func_type = arg_closure_type->func_type;

        for (unsigned i = 0; i < std::min(func_type.params.size(), arg_func_type.params.size()); i++) {
            partial_result = infer(func_type.params[i].type, arg_func_type.params[i].type);
            if (partial_result != Result::SUCCESS) {
                result = Result::ERROR;
            }
        }

        partial_result = infer(func_type.return_type, arg_func_type.return_type);
        if (partial_result != Result::SUCCESS) {
            result = Result::ERROR;
        }

        return result;
    } else {
        return Result::SUCCESS;
    }
}

Result GenericArgInference::infer_on_reference_type(sir::ReferenceType &reference_type, sir::Expr arg_type) {
    if (auto arg_reference_type = arg_type.match<sir::ReferenceType>()) {
        // If the argument is already a reference, unwrap both reference types.
        return infer(reference_type.base_type, arg_reference_type->base_type);
    } else {
        // If the argument isn't a reference, unwrap the reference type to infer from the base type.
        return infer(reference_type.base_type, arg_type);
    }
}

} // namespace banjo::sema
