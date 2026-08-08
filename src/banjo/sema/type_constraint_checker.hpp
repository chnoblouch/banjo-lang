#ifndef BANJO_SEMA_TYPE_CONSTRAINT_CHECKER_H
#define BANJO_SEMA_TYPE_CONSTRAINT_CHECKER_H

#include "banjo/sir/sir.hpp"

#include <span>
#include <vector>

namespace banjo::sema {

class SemanticAnalyzer;

class TypeConstraintChecker {

public:
    struct Substitution {
        ASTNode *ast_node;
        std::span<sir::GenericParam *> params;
        std::span<sir::Expr> args;
    };

private:
    SemanticAnalyzer &analyzer;

public:
    TypeConstraintChecker(SemanticAnalyzer &analyzer);
    void check(std::vector<Substitution> &substitutions);

private:
    void check(Substitution &substituion, unsigned index);
};

} // namespace banjo::sema

#endif
