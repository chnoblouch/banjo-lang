#ifndef BANJO_LSP_COMPLETION_ENGINE_H
#define BANJO_LSP_COMPLETION_ENGINE_H

#include "banjo/sema/completion_context.hpp"
#include "banjo/sir/sir.hpp"
#include "banjo/source/source_file.hpp"

#include <string_view>
#include <unordered_set>

namespace banjo::lsp {

class Workspace;

class CompletionEngine {

public:
    struct Request {
        SourceFile &file;
        sema::CompletionContext &context;
        std::vector<sir::Symbol> &preamble_symbols;
    };

    struct Item {
        enum class Kind {
            SIMPLE,
            FUNC_CALL_TEMPLATE,
            STRUCT_LITERAL_TEMPLATE,
            STRUCT_FIELD_TEMPLATE,
        };

        Kind kind;
        std::string_view name;
        sir::Symbol symbol;
        SourceFile *file_to_use;
    };

    struct State {
        SourceFile *file;
        std::vector<Item> items;
        std::unordered_set<sir::Symbol> symbols;
    };

private:
    struct Options {
        bool allow_values;
        bool include_parent_scopes;
        bool include_uses;
        bool create_func_call_template;
        SourceFile *file_to_use;
    };

public:
    Workspace &workspace;
    State state;

    CompletionEngine(Workspace &workspace);
    void complete(Request request);

private:
    void complete_in_block(Request request, sir::SymbolTable &symbol_table);
    void complete_after_dot(sir::Expr lhs);
    void complete_after_implicit_dot(sir::Expr type);
    void complete_in_use();
    void complete_after_use_dot(sir::UseItem &lhs);
    void complete_in_struct_literal(sir::StructLiteral &struct_literal);

    void collect_symbol_members(sir::Symbol &symbol, Options &options);
    void collect_value_members(sir::Concrete<sir::StructDef> &concrete_struct);
    void collect_value_members(sir::ProtoDef &proto_def);

    void collect_items(sir::SymbolTable &symbol_table, Options &options);
    void try_collect_item(std::string_view name, sir::Symbol symbol, Options &options);

    void add_item(Item item);
};

} // namespace banjo::lsp

#endif
