#include "return_checker.hpp"

namespace banjo {
namespace lang {
namespace sema {

ReturnChecker::ReturnChecker(SemanticAnalyzer &analyzer) : analyzer(analyzer) {}

ReturnChecker::Result ReturnChecker::check(sir::Block &block) {
    if (block.stmts.empty()) {
        return Result::RETURNS_NEVER;
    }

    Result result = Result::RETURNS_NEVER;

    for (sir::Stmt &stmt : block.stmts) {
        if (result == Result::RETURNS_ALWAYS) {
            analyzer.report_generator.report_warn_unreachable_code(stmt);
            return result;
        }

        if (stmt.is<sir::ReturnStmt>()) {
            result = Result::RETURNS_ALWAYS;
        } else if (auto block = stmt.match<sir::Block>()) {
            result = check(*block);
        } else if (auto if_stmt = stmt.match<sir::IfStmt>()) {
            result = check_if_stmt(*if_stmt);
        }
    }

    return result;
}

ReturnChecker::Result ReturnChecker::check_if_stmt(sir::IfStmt &if_stmt) {
    bool returns_always = true;
    bool has_any_return = false;

    for (sir::IfCondBranch &branch : if_stmt.cond_branches) {
        check_if_stmt_branch(*branch.block, returns_always, has_any_return);
    }

    if (if_stmt.else_branch) {
        check_if_stmt_branch(*if_stmt.else_branch->block, returns_always, has_any_return);
    } else {
        returns_always = false;
    }

    if (returns_always) {
        return Result::RETURNS_ALWAYS;
    } else {
        return has_any_return ? Result::RETURNS_SOMETIMES : Result::RETURNS_NEVER;
    }
}

void ReturnChecker::check_if_stmt_branch(sir::Block &block, bool &returns_always, bool &has_any_return) {
    switch (check(block)) {
        case Result::RETURNS_ALWAYS: has_any_return = true; break;
        case Result::RETURNS_SOMETIMES:
            returns_always = false;
            has_any_return = true;
            break;
        case Result::RETURNS_NEVER: returns_always = false; break;
    }
}

} // namespace sema
} // namespace lang
} // namespace banjo
