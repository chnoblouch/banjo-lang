#include "overload_resolver.hpp"

#include "banjo/sir/sir.hpp"

namespace banjo {
namespace lang {
namespace sema {

OverloadResolver::OverloadResolver(SemanticAnalyzer &analyzer) : analyzer(analyzer) {}

sir::FuncDef *OverloadResolver::resolve(sir::OverloadSet &set, std::span<sir::Expr> args) {
    for (sir::FuncDef *func_def : set.func_defs) {
        if (is_matching_overload(*func_def, args)) {
            return func_def;
        }
    }

    return nullptr;
}

bool OverloadResolver::is_matching_overload(sir::FuncDef &func_def, std::span<sir::Expr> args) {
    if (func_def.type.params.size() != args.size()) {
        return false;
    }

    for (unsigned i = 0; i < args.size(); i++) {
        sir::Expr arg_type = args[i].get_type();
        sir::Expr param_type = func_def.type.params[i].type;

        // FIXME: Prioritize overloads that use references.
        if (auto reference_type = param_type.match<sir::ReferenceType>()) {
            if (arg_type != reference_type->base_type && !is_coercible(arg_type, reference_type->base_type)) {
                return false;
            }
        } else {
            if (arg_type != param_type && !is_coercible(arg_type, param_type)) {
                return false;
            }
        }
    }

    return true;
}

bool OverloadResolver::is_coercible(sir::Expr arg_type, sir::Expr param_type) {
    if (auto pseudo_type = arg_type.match<sir::PseudoType>()) {
        if (pseudo_type->kind == sir::PseudoTypeKind::INT_LITERAL) {
            return param_type.is_int_type() || param_type.is_addr_like_type();
        } else if (pseudo_type->kind == sir::PseudoTypeKind::FP_LITERAL) {
            return param_type.is_fp_type();
        } else if (pseudo_type->kind == sir::PseudoTypeKind::NULL_LITERAL) {
            return param_type.is_addr_like_type();
        } else if (pseudo_type->kind == sir::PseudoTypeKind::STRING_LITERAL) {
            return param_type.is_u8_ptr() || param_type.is_symbol(analyzer.find_std_string()) ||
                   param_type.is_symbol(analyzer.find_std_string_slice());
        } else {
            return false;
        }
    } else {
        return false;
    }
}

} // namespace sema
} // namespace lang
} // namespace banjo
