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
        lang::SourceFile &file;
        lang::sema::CompletionContext &context;
        std::vector<lang::sir::Symbol> &preamble_symbols;
    };

    struct Item {
        enum class Kind {
            SIMPLE,
            FUNC_CALL_TEMPLATE,
            STRUCT_LITERAL_TEMPLATE,
            STRUCT_FIELD_TEMPLATE,
        };

        Kind kind;
        std::string name;
        lang::sir::Symbol symbol;
        lang::SourceFile *file_to_use;
    };

    struct State {
        lang::SourceFile *file;
        std::vector<Item> items;
        std::unordered_set<lang::sir::Symbol> symbols;
    };

private:
    struct Options {
        bool allow_values;
        bool include_parent_scopes;
        bool include_uses;
        bool create_func_call_template;
        lang::SourceFile *file_to_use;
    };

public:
    Workspace &workspace;
    State state;

    CompletionEngine(Workspace &workspace);
    void complete(Request request);

private:
    void complete_in_block(Request request, lang::sir::SymbolTable &symbol_table);
    void complete_after_dot(lang::sir::Expr lhs);
    void complete_after_implicit_dot(lang::sir::Expr type);
    void complete_in_use();
    void complete_after_use_dot(lang::sir::UseItem &lhs);
    void complete_in_struct_literal(lang::sir::StructLiteral &struct_literal);

    void collect_symbol_members(lang::sir::Symbol &symbol, Options &options);
    void collect_value_members(lang::sir::StructDef &struct_def);

    void collect_items(lang::sir::SymbolTable &symbol_table, Options &options);
    void try_collect_item(const std::string &name, lang::sir::Symbol symbol, Options &options);

    void add_item(Item item);
};

} // namespace banjo::lsp

#endif
