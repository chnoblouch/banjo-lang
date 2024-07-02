#ifndef LSP_COMPLETION_HANDLER_H
#define LSP_COMPLETION_HANDLER_H

#include "connection.hpp"
#include "source_manager.hpp"
#include "symbol/generics.hpp"
#include "symbol/symbol_ref.hpp"

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

    enum class CompletionKind {
        EXPR,
        AFTER_DOT,
        BEFORE_USE_TREE,
        INSIDE_USE_TREE,
    };

    SourceManager &source_manager;

public:
    CompletionHandler(SourceManager &source_manager);
    ~CompletionHandler();

    JSONValue handle(const JSONObject &params, Connection &connection);

private:
    JSONArray build_items(const CompletionContext &context);

    JSONArray complete_after_dot(lang::ASTNode *node);
    JSONArray complete_inside_use_tree_list(lang::ASTNode *node);

    JSONArray complete_module_members(lang::ASTNode *node);
    JSONArray complete_struct_members(lang::ASTNode *node);
    JSONArray complete_enum_members(lang::ASTNode *node);
    void complete_symbol_table_members(lang::ASTNode *node, lang::SymbolTable *symbol_table, JSONArray &array);
    CompletionKind get_completion_kind(lang::ASTNode *node);
    LSPCompletionItemKind get_lsp_kind(const lang::SymbolRef &symbol);
    JSONArray complete_struct_instance_members(lang::Structure *struct_);

    void complete_inside_block(lang::ASTNode *node, JSONArray &array);

    JSONObject build_symbol_item(CompletionKind kind, const std::string &name, const lang::SymbolRef &symbol);
    JSONObject build_func_item(lang::Function *func);
    JSONObject build_overload_group_item(const std::string &name);
    JSONObject build_generic_func_item(lang::GenericFunc *generic_func);
};

} // namespace lsp

} // namespace banjo

#endif
