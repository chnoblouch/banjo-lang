#include "type_alias_resolver.hpp"

#include "banjo/sema/expr_analyzer.hpp"

namespace banjo {

namespace lang {

namespace sema {

TypeAliasResolver::TypeAliasResolver(SemanticAnalyzer &analyzer) : DeclVisitor(analyzer) {}

Result TypeAliasResolver::analyze_type_alias(sir::TypeAlias &type_alias) {
    return ExprAnalyzer(analyzer).analyze_type(type_alias.type);
}

} // namespace sema

} // namespace lang

} // namespace banjo
