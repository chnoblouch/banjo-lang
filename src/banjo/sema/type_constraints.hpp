#ifndef BANJO_SEMA_TYPE_CONSTRAINTS_H
#define BANJO_SEMA_TYPE_CONSTRAINTS_H

#include "banjo/sema/semantic_analyzer.hpp"
#include "banjo/sir/sir.hpp"

namespace banjo::lang::sema {

Result check_type_constraint(
    SemanticAnalyzer &analyzer,
    ASTNode *ast_node,
    std::span<sir::GenericParam *> params,
    std::span<sir::Expr> args,
    unsigned index
);

bool primitive_implements(SemanticAnalyzer &analyzer, sir::Primitive primitive, sir::Concrete<sir::ProtoDef> proto_def);

} // namespace banjo::lang::sema

#endif
