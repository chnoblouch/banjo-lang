#ifndef LSP_COMPLETION_HANDLER_H
#define LSP_COMPLETION_HANDLER_H

#include "banjo/sir/sir.hpp"
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

    struct CompletionConfig {
        bool include_uses;
        bool append_func_parameters;
    };

    Workspace &workspace;

public:
    CompletionHandler(Workspace &workspace);
    ~CompletionHandler();

    JSONValue handle(const JSONObject &params, Connection &connection);

private:
    void build_in_block(JSONArray &items, lang::sir::SymbolTable &symbol_table);
    void build_after_dot(JSONArray &items, lang::sir::Expr &lhs);
    void build_after_use_dot(JSONArray &items, lang::sir::UseItem &lhs);

    void build_value_members(JSONArray &items, lang::sir::StructDef &struct_def);
    void build_items(const CompletionConfig &config, JSONArray &items, lang::sir::SymbolTable &symbol_table);

    void build_symbol_members(const CompletionConfig &config, JSONArray &items, lang::sir::Symbol &symbol);

    void build_item(
        const CompletionConfig &config,
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
