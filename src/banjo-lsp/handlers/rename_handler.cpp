#include "rename_handler.hpp"

#include "ast_navigation.hpp"
#include "banjo/ast/ast_child_indices.hpp"
#include "banjo/ast/ast_node.hpp"
#include "banjo/ast/ast_utils.hpp"
#include "banjo/ast/expr.hpp"
#include "banjo/symbol/local_variable.hpp"
#include "banjo/symbol/symbol.hpp"
#include "banjo/symbol/symbol_ref.hpp"
#include "banjo/utils/json.hpp"
#include "protocol_structs.hpp"
#include "symbol_ref_info.hpp"
#include "uri.hpp"

#include <unordered_map>

namespace banjo {

namespace lsp {

RenameHandler::RenameHandler(Workspace &workspace) : workspace(workspace) {}

RenameHandler::~RenameHandler() {}

JSONValue RenameHandler::handle(const JSONObject &params, Connection &connection) {
    return JSONObject{
        {"changes", JSONObject{}},
    };
}

} // namespace lsp

} // namespace banjo
