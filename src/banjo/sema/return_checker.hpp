#ifndef BANJO_SEMA_RETURN_CHECKER_H
#define BANJO_SEMA_RETURN_CHECKER_H

#include "banjo/sema/semantic_analyzer.hpp"
#include "banjo/sir/sir.hpp"

#include <cstdint>

namespace banjo {
namespace lang {
namespace sema {

class ReturnChecker {

public:
    enum class Result : std::uint8_t {
        RETURNS_ALWAYS,
        RETURNS_SOMETIMES,
        RETURNS_NEVER,
    };

private:
    SemanticAnalyzer &analyzer;

public:
    ReturnChecker(SemanticAnalyzer &analyzer);
    Result check(sir::Block &block);

private:
    Result check_if_stmt(sir::IfStmt &if_stmt);
    void check_if_stmt_branch(sir::Block &block, bool &returns_always, bool &has_any_return);
};

} // namespace sema
} // namespace lang
} // namespace banjo

#endif
