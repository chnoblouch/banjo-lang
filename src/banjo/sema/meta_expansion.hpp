#ifndef BANJO_SEMA_META_EXPANSION_H
#define BANJO_SEMA_META_EXPANSION_H

#include "banjo/sema/semantic_analyzer.hpp"
#include "banjo/sir/sir.hpp"

namespace banjo {

namespace lang {

namespace sema {

class MetaExpansion {

private:
    SemanticAnalyzer &analyzer;

public:
    MetaExpansion(SemanticAnalyzer &analyzer);
    void run(const std::vector<sir::Module *> &mods);
    void run_on_decl_block(sir::DeclBlock &decl_block);

    void evaluate_meta_if_stmt(sir::DeclBlock &decl_block, unsigned &index);
    void expand(sir::DeclBlock &decl_block, unsigned &index, sir::DeclBlock &body);

    void evaluate_meta_if_stmt(sir::Block &block, unsigned &index);
    void evaluate_meta_for_stmt(sir::Block &block, unsigned &index);
    void expand(sir::Block &block, unsigned &index, sir::Block &body);

    Result evaluate_meta_for_range(sir::Expr range, std::vector<sir::Expr> &out_values);
};

} // namespace sema

} // namespace lang

} // namespace banjo

#endif
