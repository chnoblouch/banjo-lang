#ifndef BANJO_SEMA_POINTER_ESCAPE_CHECKER_H
#define BANJO_SEMA_POINTER_ESCAPE_CHECKER_H

#include "banjo/sema/semantic_analyzer.hpp"

namespace banjo {
namespace lang {
namespace sema {

class PointerEscapeChecker {

private:
    SemanticAnalyzer &analyzer;

public:
    PointerEscapeChecker(SemanticAnalyzer &analyzer);
    void check_return_stmt(sir::ReturnStmt &return_stmt);

private:
    void check_escaping_value(sir::Stmt stmt, sir::Expr value);
};

} // namespace sema
} // namespace lang
} // namespace banjo

#endif
