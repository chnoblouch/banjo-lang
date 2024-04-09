#ifndef LSP_REFERENCES_HANDLER_H
#define LSP_REFERENCES_HANDLER_H

#include "connection.hpp"
#include "source_manager.hpp"

namespace lsp {

class ReferencesHandler : public RequestHandler {

private:
    SourceManager &source_manager;

public:
    ReferencesHandler(SourceManager &source_manager);
    ~ReferencesHandler();

    JSONValue handle(const JSONObject &params, Connection &connection);
};

} // namespace lsp

#endif
