#ifndef LSP_DEFINITION_HANDLER_H
#define LSP_DEFINITION_HANDLER_H

#include "ast/ast_module.hpp"
#include "connection.hpp"
#include "source_manager.hpp"
#include "symbol/enumeration.hpp"

namespace banjo {

namespace lsp {

class DefinitionHandler : public RequestHandler {

private:
    SourceManager &source_manager;

public:
    DefinitionHandler(SourceManager &source_manager);
    ~DefinitionHandler();

    JSONValue handle(const JSONObject &params, Connection &connection);

private:
    JSONObject build_def(lang::ASTModule *module_, lang::TextRange range, lang::TextRange selection_range);
};

} // namespace lsp

} // namespace banjo

#endif
