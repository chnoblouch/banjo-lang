#include "function_resolution.hpp"

#include "ast/ast_child_indices.hpp"
#include "reports/report_utils.hpp"
#include "sema/expr_analyzer.hpp"
#include "sema/generics_arg_deduction.hpp"
#include "sema/generics_instantiator.hpp"

#include <cassert>

namespace lang {

FunctionResolution::FunctionResolution(
    SemanticAnalyzerContext &context,
    ASTNode *node,
    const SymbolRef &symbol,
    SymbolUsage usage
)
  : context(context),
    node(node),
    symbol(symbol),
    usage(usage) {}

SymbolRef FunctionResolution::resolve() {
    switch (symbol.get_kind()) {
        case SymbolKind::FUNCTION: return resolve_func();
        case SymbolKind::GROUP: return resolve_overload();
        case SymbolKind::GENERIC_FUNC: return resolve_generic_func();
        default: return {};
    }
}

SymbolRef FunctionResolution::resolve_func() {
    Function *func = symbol.get_func();
    bool valid = true;

    if (usage.arg_list) {
        unsigned param_offset = func->is_method() ? 1 : 0;
        unsigned num_params = func->get_type().param_types.size();
        unsigned num_args = usage.arg_list->get_children().size();

        bool num_args_matches = num_params - param_offset == num_args;

        if (!num_args_matches) {
            // TODO: better error message
            context.register_error("argument count does not match parameter count", node);
            return {};
        }

        for (unsigned int i = 0; i < num_args; i++) {
            ASTNode *arg = usage.arg_list->get_child(i);

            ExprAnalyzer arg_checker(arg, context);
            if (num_args_matches) {
                arg_checker.set_expected_type(func->get_type().param_types[i + param_offset]);
            }

            if (!arg_checker.check()) {
                valid = false;
            }
        }
    }

    if (!valid) {
        return {};
    }

    return func;
}

SymbolRef FunctionResolution::resolve_overload() {
    if (usage.arg_list) {
        std::optional<std::vector<DataType *>> arg_types = analyze_arg_types();
        if (!arg_types) {
            return {};
        }

        for (const SymbolRef &symbol : symbol.get_group()->symbols) {
            if (symbol.get_kind() != SymbolKind::FUNCTION) {
                continue;
            }

            Function *func = symbol.get_func();
            unsigned params_start = func->is_method() ? 1 : 0;

            if (DataType::equal(func->get_type().param_types, *arg_types, params_start)) {
                return func;
            }
        }

        report_unresolved_overload(*arg_types);
        return {};
    } else {
        return symbol.get_group()->symbols[0].get_func();
    }
}

SymbolRef FunctionResolution::resolve_generic_func() {
    if (!usage.arg_list) {
        // If there is no argument list, don't resolve the generic function symbol
        return symbol;
    }

    std::optional<std::vector<DataType *>> arg_types = analyze_arg_types();
    if (!arg_types) {
        return {};
    }

    GenericFunc *generic_func = symbol.get_generic_func();
    ASTNode *params_node = generic_func->get_node()->get_child(GENERIC_FUNC_PARAMS);
    const std::vector<ASTNode *> &param_nodes = params_node->get_children();

    // if (arg_types->size() != param_nodes.size()) {
    //     return {};
    // }

    bool has_sequence = GenericsUtils::has_sequence(generic_func);
    GenericsArgDeduction deduction(context.get_type_manager(), generic_func->get_generic_params(), has_sequence);
    std::optional<std::vector<DataType *>> args = deduction.deduce(param_nodes, *arg_types);

    if (!args) {
        return {};
    }

    GenericFuncInstance *instance = GenericsInstantiator(context).instantiate_func(generic_func, *args);
    return instance->entity;
}

std::optional<std::vector<DataType *>> FunctionResolution::analyze_arg_types() {
    assert(usage.arg_list);

    std::vector<DataType *> arg_types;
    bool valid = true;

    for (ASTNode *arg_node : usage.arg_list->get_children()) {
        ExprAnalyzer arg_checker(arg_node, context);
        if (!arg_checker.check()) {
            valid = false;
        }
        arg_types.push_back(arg_checker.get_type());
    }

    if (valid) {
        return arg_types;
    } else {
        return {};
    }
}

void FunctionResolution::report_unresolved_overload(std::vector<DataType *> &arg_types) {
    Report &report = context.register_error(node->get_range());
    report.set_message(ReportText(ReportText::ERR_NO_VALUE).format(node).str());

    std::string type_note = "argument types: (" + ReportUtils::params_to_string(arg_types) + ")";
    report.add_note({type_note});

    for (const SymbolRef &overload : symbol.get_group()->symbols) {
        Function *func = overload.get_func();

        std::string params = ReportUtils::params_to_string(func->get_type().param_types);
        std::string text = "candidate " + func->get_name() + "(" + params + ") not viable";
        TextRange range = func->get_node()->get_child(FUNC_NAME)->get_range();
        report.add_note({text, SourceLocation{func->get_module()->get_path(), range}});
    }
}

} // namespace lang
