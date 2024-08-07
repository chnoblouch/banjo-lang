#ifndef META_EXPANSION_H
#define META_EXPANSION_H

#include "banjo/sema2/semantic_analyzer.hpp"
#include "banjo/sir/sir.hpp"

namespace banjo {

namespace lang {

namespace sema {

class MetaExpansion {

private:
    SemanticAnalyzer &analyzer;

public:
    MetaExpansion(SemanticAnalyzer &analyzer);
    void run();
    void run_on_decl_block(sir::DeclBlock &decl_block);

private:
    void evaluate_meta_if_stmt(sir::DeclBlock &decl_block, unsigned &index);
    void expand(sir::DeclBlock &decl_block, unsigned &index, sir::MetaBlock &meta_block);
};

} // namespace sema

} // namespace lang

} // namespace banjo

#endif
