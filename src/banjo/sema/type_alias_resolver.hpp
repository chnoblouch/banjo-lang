#ifndef TYPE_ALIAS_RESOLVER_H
#define TYPE_ALIAS_RESOLVER_H

#include "banjo/sema/decl_visitor.hpp"
#include "banjo/sema/semantic_analyzer.hpp"
#include "banjo/sir/sir.hpp"

#include <set>

namespace banjo {

namespace lang {

namespace sema {

class TypeAliasResolver final : public DeclVisitor {

private:
    std::set<const sir::TypeAlias *> resolved_type_aliases;

public:
    TypeAliasResolver(SemanticAnalyzer &analyzer);
    Result analyze_type_alias(sir::TypeAlias &type_alias) override;
};

} // namespace sema

} // namespace lang

} // namespace banjo

#endif
