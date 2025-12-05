#ifndef BANJO_LSP_HANDLERS_COMPLETION_ITEM_RESOLVE_HANDLER_H
#define BANJO_LSP_HANDLERS_COMPLETION_ITEM_RESOLVE_HANDLER_H

#include "banjo/ast/ast_node.hpp"
#include "banjo/source/source_file.hpp"
#include "banjo/source/text_range.hpp"

#include "connection.hpp"
#include "workspace.hpp"

#include <string>

namespace banjo::lsp {

class CompletionItemResolveHandler : public RequestHandler {

private:
    struct TextInsertion {
        lang::TextPosition position;
        std::string text;
    };

    Workspace &workspace;

public:
    CompletionItemResolveHandler(Workspace &workspace);

    JSONValue handle(const JSONObject &params, Connection &connection);

private:
    TextInsertion insert_line_after(lang::SourceFile &file, lang::ASTNode *node, const std::string &text);
};

} // namespace banjo::lsp

#endif
