#ifndef BANJO_LSP_HANDLERS_REFERENCES_HANDLER_H
#define BANJO_LSP_HANDLERS_REFERENCES_HANDLER_H

#include "connection.hpp"
#include "workspace.hpp"

namespace banjo::lsp {

class ReferencesHandler : public RequestHandler {

private:
    Workspace &workspace;

public:
    ReferencesHandler(Workspace &workspace);
    ~ReferencesHandler();

    json::Value handle(const json::Object &params, Connection &connection);

private:
    const SourceFile *find_file(const json::Object &params);
    const SymbolRef *find_symbol(const SourceFile &file, const json::Object &params);
};

} // namespace banjo::lsp

#endif
