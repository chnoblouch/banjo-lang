#include "type_alias_resolver.hpp"

#include "banjo/sema/expr_analyzer.hpp"
#include "banjo/sir/sir.hpp"
#include <algorithm>

namespace banjo {

namespace lang {

namespace sema {

TypeAliasResolver::TypeAliasResolver(SemanticAnalyzer &analyzer) : DeclVisitor(analyzer) {}

Result TypeAliasResolver::analyze_type_alias(sir::TypeAlias &type_alias) {
    if (type_alias.stage >= sir::SemaStage::BODY) {
        return Result::SUCCESS;
    }

    if (std::ranges::count(analyzer.decl_stack, sir::Decl{&type_alias})) {
        return Result::DEF_CYCLE;
    }

    analyzer.decl_stack.push_back(&type_alias);
    ExprAnalyzer expr_analyzer{analyzer, ExprAnalyzer::ANALYZE_SYMBOL_INTERFACES};
    Result result = expr_analyzer.analyze_type(type_alias.type);
    analyzer.decl_stack.pop_back();

    type_alias.stage = sir::SemaStage::BODY;
    return result;
}

} // namespace sema

} // namespace lang

} // namespace banjo
