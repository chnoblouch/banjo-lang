#ifndef BANJO_LSP_HANDLERS_RENAME_HANDLER_H
#define BANJO_LSP_HANDLERS_RENAME_HANDLER_H

#include "connection.hpp"
#include "workspace.hpp"

namespace banjo::lsp {

class RenameHandler : public RequestHandler {

private:
    Workspace &workspace;

public:
    RenameHandler(Workspace &workspace);
    ~RenameHandler();

    json::Value handle(const json::Object &params, Connection &connection);

private:
    const SourceFile *find_file(const json::Object &params);
    const SymbolRef *find_symbol(const SourceFile &file, const json::Object &params);
};

} // namespace banjo::lsp

#endif
