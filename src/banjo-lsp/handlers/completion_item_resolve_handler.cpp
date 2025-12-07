#include "completion_item_resolve_handler.hpp"

#include "banjo/ast/ast_node.hpp"
#include "banjo/config/config.hpp"
#include "banjo/lexer/token.hpp"
#include "banjo/source/text_range.hpp"
#include "banjo/utils/json.hpp"

#include "completion_engine.hpp"
#include "protocol_structs.hpp"
#include "workspace.hpp"

#include <algorithm>
#include <vector>

namespace banjo::lsp {

using namespace lang;

CompletionItemResolveHandler::CompletionItemResolveHandler(Workspace &workspace) : workspace(workspace) {}

JSONValue CompletionItemResolveHandler::handle(const JSONObject &params, Connection & /*connection*/) {
    std::optional<unsigned> index = params.try_get_int("data");

    if (!index || *index >= workspace.completion_engine.state.items.size()) {
        return params;
    }

    SourceFile &cur_file = *workspace.completion_engine.state.file;
    CompletionEngine::Item &item = workspace.completion_engine.state.items[*index];

    if (!item.file_to_use) {
        return params;
    }

    std::string name = item.symbol.get_name();
    std::string_view mod_path = item.file_to_use->mod_path.to_string();
    std::string use_text = "use " + std::string{mod_path} + '.' + name;

    if (Config::instance().optional_semicolons) {
        use_text += "\n";
    } else {
        use_text += ";\n";
    }

    ASTNode *last_use = nullptr;

    for (sir::Decl &decl : cur_file.sir_mod->block.decls) {
        if (auto use_decl = decl.match<sir::UseDecl>()) {
            last_use = use_decl->ast_node;
        } else {
            break;
        }
    }

    TextPosition insert_pos;

    if (last_use) {
        TextInsertion insertion = insert_line_after(cur_file, last_use, use_text);
        insert_pos = insertion.position;
        use_text = insertion.text;
    } else {
        insert_pos = 0;

        std::span<Token> attached_tokens = cur_file.tokens.get_attached_tokens(0);

        if (attached_tokens.size() == 0 || !attached_tokens[0].is(TKN_WHITESPACE) ||
            std::ranges::count(attached_tokens[0].value, '\n') == 0) {
            use_text += "\n";
        }
    }

    JSONObject edit{
        {"range", ProtocolStructs::range_to_lsp(cur_file.get_content(), {insert_pos, insert_pos})},
        {"newText", use_text},
    };

    JSONObject result = params;
    result.add("additionalTextEdits", JSONArray{edit});
    return result;
}

CompletionItemResolveHandler::TextInsertion CompletionItemResolveHandler::insert_line_after(
    lang::SourceFile &file,
    lang::ASTNode *node,
    const std::string &text
) {
    unsigned last_token = file.tokens.last_token_index(node);
    std::span<Token> attached_tokens = file.tokens.get_attached_tokens(last_token + 1);

    if (attached_tokens.empty()) {
        return TextInsertion{
            .position = node->range.end,
            .text = '\n' + text,
        };
    }

    std::optional<unsigned> comment_token;

    if (attached_tokens[0].is(TKN_COMMENT)) {
        comment_token = 0;
    } else if (attached_tokens.size() >= 2 && attached_tokens[1].is(TKN_COMMENT)) {
        bool comment_on_same_line = true;

        for (char c : attached_tokens[0].value) {
            if (c == '\n') {
                comment_on_same_line = false;
                break;
            }
        }

        if (comment_on_same_line) {
            comment_token = 1;
        }
    }

    Token whitespace_token = attached_tokens[comment_token ? *comment_token + 1 : 0];
    std::string::size_type first_new_line = whitespace_token.value.find_first_of('\n');

    TextPosition position;

    if (first_new_line == std::string::npos) {
        position = node->range.end;
    } else {
        position = whitespace_token.position + first_new_line + 1;
    }

    return TextInsertion{
        .position = position,
        .text = text,
    };
}

} // namespace banjo::lsp
