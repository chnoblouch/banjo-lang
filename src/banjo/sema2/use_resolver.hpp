#ifndef USE_RESOLVER_H
#define USE_RESOLVER_H

#include "banjo/sema2/semantic_analyzer.hpp"
#include "banjo/sir/sir.hpp"

namespace banjo {

namespace lang {

namespace sema {

class UseResolver {

private:
    SemanticAnalyzer &analyzer;

public:
    UseResolver(SemanticAnalyzer &analyzer);
    void resolve();

private:
    void resolve_in_block(sir::DeclBlock &decl_block);
    void resolve_use_item(sir::UseItem &use_item, sir::Symbol &symbol);
    void resolve_use_ident(sir::UseIdent &use_ident, sir::Symbol &symbol);
    void resolve_use_rebind(sir::UseRebind &use_rebind, sir::Symbol &symbol);
    void resolve_use_dot_expr(sir::UseDotExpr &use_dot_expr, sir::Symbol &symbol);
    void resolve_use_list(sir::UseList &use_list, sir::Symbol &symbol);

    sir::Symbol resolve_symbol(sir::Ident &ident, sir::Symbol &symbol);
    sir::Symbol resolve_module(sir::Ident &ident, sir::Symbol &symbol);
};

} // namespace sema

} // namespace lang

} // namespace banjo

#endif
