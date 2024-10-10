#ifndef LSP_COMPLETION_HANDLER_H
#define LSP_COMPLETION_HANDLER_H

#include "banjo/sir/sir.hpp"
#include "banjo/symbol/generics.hpp"
#include "banjo/symbol/symbol_ref.hpp"
#include "connection.hpp"
#include "workspace.hpp"

namespace banjo {

namespace lsp {

struct CompletionContext {
    lang::ASTModule *module_node;
    lang::ASTNode *node;
};

class CompletionHandler : public RequestHandler {

private:
    enum LSPCompletionItemKind {
        METHOD = 2,
        FUNCTION = 3,
        FIELD = 5,
        VARIABLE = 6,
        MODULE = 9,
        ENUM = 13,
        KEYWORD = 14,
        ENUM_MEMBER = 20,
        CONSTANT = 21,
        STRUCT = 22,
    };

    enum class CompletionKind {
        EXPR,
        AFTER_DOT,
        BEFORE_USE_TREE,
        INSIDE_USE_TREE,
    };

    Workspace &workspace;

public:
    CompletionHandler(Workspace &workspace);
    ~CompletionHandler();

    JSONValue handle(const JSONObject &params, Connection &connection);

private:
    void build_items(JSONArray &items, const lang::sir::SymbolTable &symbol_table);
    void build_item(JSONArray &items, std::string_view name, const lang::sir::Symbol &symbol);
    JSONObject build_item(std::string_view name, const lang::sir::FuncType &type, bool is_method);
    void build_item(JSONArray &items, std::string_view name, const lang::sir::OverloadSet &overload_set);

    JSONObject create_simple_item(std::string_view name, LSPCompletionItemKind kind);
};

} // namespace lsp

} // namespace banjo

#endif
