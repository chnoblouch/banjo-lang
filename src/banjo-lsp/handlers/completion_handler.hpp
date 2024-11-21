#ifndef LSP_COMPLETION_HANDLER_H
#define LSP_COMPLETION_HANDLER_H

#include "banjo/sir/sir.hpp"
#include "banjo/symbol/generics.hpp"
#include "banjo/symbol/symbol_ref.hpp"
#include "banjo/utils/json.hpp"
#include "connection.hpp"
#include "workspace.hpp"

namespace banjo {

namespace lsp {

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

    Workspace &workspace;

public:
    CompletionHandler(Workspace &workspace);
    ~CompletionHandler();

    JSONValue handle(const JSONObject &params, Connection &connection);

private:
    void build_after_dot(lang::sema::CompletionContext &context, JSONArray &items, lang::sir::Expr &lhs);
    void build_after_use_dot(lang::sema::CompletionContext &context, JSONArray &items, lang::sir::UseItem &lhs);

    void build_symbol_members(lang::sema::CompletionContext &context, JSONArray &items, lang::sir::Symbol &symbol);

    void build_value_members(
        lang::sema::CompletionContext &context,
        JSONArray &items,
        const lang::sir::SymbolTable &symbol_table
    );

    void build_items(
        lang::sema::CompletionContext &context,
        JSONArray &items,
        const lang::sir::SymbolTable &symbol_table
    );

    void build_item(
        lang::sema::CompletionContext &context,
        JSONArray &items,
        std::string_view name,
        const lang::sir::Symbol &symbol
    );

    JSONObject build_item(std::string_view name, const lang::sir::FuncType &type, bool is_method);
    void build_item(JSONArray &items, std::string_view name, const lang::sir::OverloadSet &overload_set);

    JSONObject create_simple_item(std::string_view name, LSPCompletionItemKind kind);
};

} // namespace lsp

} // namespace banjo

#endif
