#include "generic_arg_deduction.hpp"

#include <cassert>

namespace banjo {

namespace lang {

namespace sema {

GenericArgDeduction::GenericArgDeduction(SemanticAnalyzer &analyzer) : analyzer(analyzer) {}

std::optional<std::vector<sir::Expr>> GenericArgDeduction::deduce(
    const std::vector<sir::GenericParam> &generic_params,
    const std::vector<sir::Expr> &args
) {
    std::vector<sir::Expr> generic_args;
    generic_args.resize(generic_params.size());

    for (unsigned i = 0; i < args.size(); i++) {
        assert(args[i].get_type());
        generic_args[i] = args[i].get_type();
    }

    return generic_args;
}

} // namespace sema

} // namespace lang

} // namespace banjo
