#ifndef BANJO_SEMA_TYPE_CONCRETIZER_H
#define BANJO_SEMA_TYPE_CONCRETIZER_H

#include "banjo/sema/decl_visitor.hpp"
#include "banjo/sema/semantic_analyzer.hpp"
#include "banjo/sir/sir.hpp"

namespace banjo::sema {

class TypeConcretizer final : public DeclVisitor {

public:
    TypeConcretizer(SemanticAnalyzer &analyzer);
    void analyze_decl_block(sir::DeclBlock &decl_block) override;

private:
    void concretize_type(sir::DefaultMethod &impl);
};

} // namespace banjo::sema

#endif
