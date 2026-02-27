#include "type_constraints.hpp"

namespace banjo::lang::sema {

Result check_type_constraint(SemanticAnalyzer &analyzer, ASTNode *ast_node, sir::Expr arg, sir::GenericParam &param) {
    if (!param.constraint) {
        return Result::SUCCESS;
    }

    bool satisfied = false;

    if (auto struct_def = arg.match_symbol<sir::StructDef>()) {
        if (struct_def->has_impl_for(param.constraint.as_symbol<sir::ProtoDef>())) {
            satisfied = true;
        }
    }

    if (satisfied) {
        return Result::SUCCESS;
    } else {
        analyzer.report_generator.report_err_constraint_not_satisfied(ast_node, arg, param);
        return Result::ERROR;
    }
}

} // namespace banjo::lang::sema
