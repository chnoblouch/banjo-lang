#ifndef BANJO_SEMA_TYPE_ALIAS_RESOLVER_H
#define BANJO_SEMA_TYPE_ALIAS_RESOLVER_H

#include "banjo/sema/decl_visitor.hpp"
#include "banjo/sema/semantic_analyzer.hpp"
#include "banjo/sir/sir.hpp"

namespace banjo::lang::sema {

class TypeAliasResolver final : public DeclVisitor {

public:
    TypeAliasResolver(SemanticAnalyzer &analyzer);
    Result analyze_type_alias(sir::TypeAlias &type_alias) override;
};

} // namespace banjo::lang::sema

#endif
