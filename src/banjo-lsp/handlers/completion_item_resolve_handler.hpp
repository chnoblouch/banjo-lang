#ifndef BANJO_LSP_HANDLERS_COMPLETION_ITEM_RESOLVE_HANDLER_H
#define BANJO_LSP_HANDLERS_COMPLETION_ITEM_RESOLVE_HANDLER_H

#include "banjo/ast/ast_module.hpp"
#include "banjo/ast/ast_node.hpp"
#include "banjo/sir/sir.hpp"
#include "banjo/source/source_file.hpp"
#include "banjo/source/text_range.hpp"
#include "banjo/utils/fixed_vector.hpp"

#include "connection.hpp"
#include "workspace.hpp"

#include <string>

namespace banjo::lsp {

class CompletionItemResolveHandler : public RequestHandler {

private:
    struct TextInsertion {
        TextPosition position;
        std::string text;
    };

    struct UseInsertionPoint {
        sir::UseDotExpr *top_level_dot_expr;
        ASTNode *ast_node;
        unsigned common_ancestor;
    };

    Workspace &workspace;

public:
    CompletionItemResolveHandler(Workspace &workspace);

    JSONValue handle(const JSONObject &params, Connection &connection);

private:
    FixedVector<TextInsertion, 2> try_modify_use(
        SourceFile &cur_file,
        CompletionEngine::Item &item,
        sir::UseDecl &use_decl
    );

    std::optional<UseInsertionPoint> find_insertion_point(
        sir::Module &mod,
        sir::UseItem &use_item,
        sir::UseDotExpr *top_level_dot_expr
    );

    TextInsertion insert_line_after(SourceFile &file, ASTNode *node, const std::string &text);
    JSONObject serialize_insertion(SourceFile &file, TextInsertion insertion);
};

} // namespace banjo::lsp

#endif
