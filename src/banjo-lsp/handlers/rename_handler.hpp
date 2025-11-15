#ifndef BANJO_LSP_HANDLERS_RENAME_HANDLER_H
#define BANJO_LSP_HANDLERS_RENAME_HANDLER_H

#include "connection.hpp"
#include "workspace.hpp"

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
    const lang::SourceFile *find_file(const JSONObject &params);
    const SymbolRef *find_symbol(const lang::SourceFile &file, const JSONObject &params);
};

} // namespace lsp

} // namespace banjo

#endif
