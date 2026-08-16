#ifndef BANJO_LSP_HANDLERS_COMPLETION_HANDLER_H
#define BANJO_LSP_HANDLERS_COMPLETION_HANDLER_H

#include "banjo/sir/sir.hpp"
#include "banjo/utils/json.hpp"
#include "completion_engine.hpp"
#include "connection.hpp"
#include "workspace.hpp"

namespace banjo::lsp {

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
        TYPE_PARAMETER = 25,
    };

    struct CompletionConfig {
        bool allow_values;
        bool include_parent_scopes;
        bool include_uses;
        bool append_func_parameters;
    };

    Workspace &workspace;

public:
    CompletionHandler(Workspace &workspace);
    json::Value handle(const json::Object &params, Connection &connection);

private:
    json::Object serialize_item(unsigned index, CompletionEngine::Item item);
    json::Object serialize_simple_item(unsigned index, CompletionEngine::Item item, LSPCompletionItemKind kind);
    json::Object serialize_func_call_template(unsigned index, CompletionEngine::Item &item, const sir::FuncType &type);
    json::Object serialize_struct_literal_template(unsigned index, CompletionEngine::Item &item);
    json::Object serialize_struct_field_template(unsigned index, CompletionEngine::Item &item);
};

} // namespace banjo::lsp

#endif
