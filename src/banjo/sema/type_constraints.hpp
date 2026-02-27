#ifndef BANJO_SEMA_TYPE_CONSTRAINTS_H
#define BANJO_SEMA_TYPE_CONSTRAINTS_H

#include "banjo/sema/semantic_analyzer.hpp"
#include "banjo/sir/sir.hpp"

namespace banjo::lang::sema {

Result check_type_constraint(SemanticAnalyzer &analyzer, ASTNode *ast_node, sir::Expr arg, sir::GenericParam &param);

}

#endif
