#ifndef META_LOWERER_H
#define META_LOWERER_H

#include "banjo/sema/meta_evaluator.hpp"
#include "banjo/sema/semantic_analyzer_context.hpp"

#include <optional>
#include <variant>
#include <vector>

namespace banjo {

namespace lang {

struct MetaExpansion {
    ASTNode *parent;
    unsigned index;
};

class MetaLowerer {

private:
    SemanticAnalyzerContext &context;
    DataType *expected_type = nullptr;
    std::vector<MetaExpansion> expansions;

public:
    MetaLowerer(SemanticAnalyzerContext &context);
    void set_expected_type(DataType *expected_type) { this->expected_type = expected_type; }
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

} // namespace banjo

#endif
