#include "type_constraint_checker.hpp"

#include "banjo/sema/semantic_analyzer.hpp"
#include "banjo/sir/type_constraints.hpp"

#include <vector>

namespace banjo::sema {

TypeConstraintChecker::TypeConstraintChecker(SemanticAnalyzer &analyzer) : analyzer{analyzer} {}

void TypeConstraintChecker::check(std::vector<Substitution> &substitutions) {
    for (Substitution &substitution : substitutions) {
        for (unsigned i = 0; i < substitution.args.size(); i++) {
            check(substitution, i);
        }
    }
}

void TypeConstraintChecker::check(Substitution &substitution, unsigned index) {
    sir::GenericParam &param = *substitution.params[index];
    sir::Expr arg = substitution.args[index];

    if (param.kind != sir::GenericParamKind::TYPE) {
        return; // Result::SUCCESS;
    }

    utils::Arena arena;
    sir::Specializer specializer{arena, substitution.params, substitution.args};

    if (sir::satisfies_type_constraint(param.constraint, arg, &specializer)) {
        return; // Result::SUCCESS;
    } else {
        ASTNode *ast_node = substitution.ast_node;
        if (!ast_node) {
            ast_node = arg.get_ast_node();
        }

        analyzer.report_generator.report_err_constraint_not_satisfied(ast_node, arg, param);
        return; // Result::ERROR;
    }
}

} // namespace banjo::sema
