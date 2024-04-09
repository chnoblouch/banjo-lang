#ifndef META_EVALUATOR_H
#define META_EVALUATOR_H

#include "ast/expr.hpp"
#include "sema/semantic_analyzer_context.hpp"
#include "utils/large_int.hpp"
#include <vector>

namespace lang {

struct MetaValue {
    SemaResult result;
    Expr *node;
    bool owning;

    MetaValue(SemaResult result, Expr *node, bool owning) : result(result), node(node), owning(owning) {}
    MetaValue(SemaResult result) : result(result), node(nullptr), owning(false) {}
    MetaValue() {}
    MetaValue(const MetaValue &) = delete;
    MetaValue(MetaValue &&other) noexcept;
    ~MetaValue();

    MetaValue &operator=(const MetaValue &) = delete;
    MetaValue &operator=(MetaValue &&other) noexcept;
};

struct MetaSymbol {
    ASTNode *decl_node;
    Expr *value_node;
};

class MetaEvaluator {

private:
    SemanticAnalyzerContext &context;
    std::vector<Expr *> tmp_nodes;

public:
    MetaEvaluator(SemanticAnalyzerContext &context);
    MetaValue eval_expr(Expr *node);

private:
    MetaValue eval_operator(Expr *node);
    MetaValue eval_not(Expr *node);
    MetaValue eval_location(Expr *node);
    MetaValue eval_type(Expr *node);
    MetaValue eval_range(Expr *node);

    LargeInt to_int(const MetaValue &value);
    double to_fp(const MetaValue &value);
    bool to_bool(const MetaValue &value);

    MetaValue from_int(LargeInt value);
    MetaValue from_fp(double value);
    MetaValue from_bool(bool value);

    MetaSymbol resolve_ident(const std::string &ident, ASTNode *block);
};

} // namespace lang

#endif
