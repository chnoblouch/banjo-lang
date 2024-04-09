#ifndef META_LOWERER_H
#define META_LOWERER_H

#include "sema/meta_evaluator.hpp"
#include "sema/semantic_analyzer_context.hpp"

#include <optional>
#include <variant>
#include <vector>

namespace lang {

struct MetaExpansion {
    ASTNode *parent;
    unsigned index;
};

class MetaLowerer {

private:
    SemanticAnalyzerContext &context;
    std::vector<MetaExpansion> expansions;

public:
    MetaLowerer(SemanticAnalyzerContext &context);
    void lower(ASTNode *node);
    bool lower_meta_if(ASTNode *node, unsigned index_in_parent);
    bool lower_meta_for(ASTNode *node, unsigned index_in_parent);
    bool lower_meta_field_access(ASTNode *node);
    bool lower_meta_method_call(ASTNode *node);

    const std::vector<MetaExpansion> &get_expansions() const { return expansions; }

private:
    void expand(ASTNode *parent, unsigned index, ASTNode *block_node);
    void replace_identifier_rec(ASTNode *node, const std::string &identifier, ASTNode *replacement);
};

} // namespace lang

#endif
