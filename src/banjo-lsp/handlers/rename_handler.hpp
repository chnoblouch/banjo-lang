#ifndef LSP_RENAME_HANDLER_H
#define LSP_RENAME_HANDLER_H

#include "banjo/ast/ast_node.hpp"
#include "banjo/symbol/symbol.hpp"
#include "connection.hpp"
#include "workspace.hpp"

#include <optional>

namespace banjo {

namespace lsp {

class RenameHandler : public RequestHandler {

private:
    Workspace &workspace;

public:
    RenameHandler(Workspace &workspace);
    ~RenameHandler();

    JSONValue handle(const JSONObject &params, Connection &connection);

private:
    const File *find_file(const JSONObject &params);
    const SymbolRef *find_symbol(const File &file, const JSONObject &params);
};

} // namespace lsp

} // namespace banjo

#endif
