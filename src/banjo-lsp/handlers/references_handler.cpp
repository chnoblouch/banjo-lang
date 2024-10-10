#include "references_handler.hpp"

#include "ast_navigation.hpp"
#include "banjo/ast/ast_node.hpp"
#include "banjo/ast/expr.hpp"
#include "banjo/symbol/symbol.hpp"
#include "banjo/symbol/symbol_ref.hpp"
#include "banjo/utils/json.hpp"
#include "protocol_structs.hpp"
#include "symbol_ref_info.hpp"
#include "uri.hpp"

namespace banjo {

namespace lsp {

ReferencesHandler::ReferencesHandler(Workspace &workspace) : workspace(workspace) {}

ReferencesHandler::~ReferencesHandler() {}

JSONValue ReferencesHandler::handle(const JSONObject &params, Connection &connection) {
    return JSONArray{};
}

} // namespace lsp

} // namespace banjo
