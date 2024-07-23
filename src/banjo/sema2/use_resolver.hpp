#ifndef USE_RESOLVER_H
#define USE_RESOLVER_H

#include "banjo/sema2/semantic_analyzer.hpp"
#include "banjo/sir/sir.hpp"

namespace banjo {

namespace lang {

namespace sema {

class UseResolver {

private:
    struct PathContext {
        sir::Module *mod;
        sir::SymbolTable *symbol_table;
    };

    SemanticAnalyzer analyzer;

public:
    UseResolver(SemanticAnalyzer &analyzer);
    void resolve();

private:
    void resolve_in_block(sir::DeclBlock &decl_block);
    void resolve_use_item(sir::UseItem &use_item, PathContext &inout_ctx);
    void resolve_use_ident(sir::UseIdent &use_ident, PathContext &inout_ctx);
    void resolve_use_rebind(sir::UseRebind &use_rebind, PathContext &inout_ctx);
    void resolve_use_dot_expr(sir::UseDotExpr &use_dot_expr, PathContext &inout_ctx);
    void resolve_use_list(sir::UseList &use_list, PathContext &inout_ctx);

    sir::Symbol resolve_symbol(const std::string &name, PathContext &inout_ctx);
};

} // namespace sema

} // namespace lang

} // namespace banjo

#endif
