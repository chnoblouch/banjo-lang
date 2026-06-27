#include "completion_item_resolve_handler.hpp"

#include "banjo/ast/ast_node.hpp"
#include "banjo/config/config.hpp"
#include "banjo/lexer/token.hpp"
#include "banjo/sir/sir.hpp"
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

    for (sir::Decl &decl : cur_file.sir_mod->block.decls) {
        if (auto use_decl = decl.match<sir::UseDecl>()) {
            // TODO: Consider comments.

            std::optional<UseInsertionPoint> point =
                find_insertion_point(*item.file_to_use->sir_mod, use_decl->root_item, nullptr);

            if (!point) {
                continue;
            }

            ModulePath stripped_path = item.file_to_use->mod_path.strip(point->common_ancestor);
            std::string path;

            if (stripped_path.is_empty()) {
                path = name;
            } else {
                path = std::string{stripped_path.to_string()} + '.' + name;
            }

            ASTNode *dot_expr = point->top_level_dot_expr->ast_node;
            ASTNode *rhs = dot_expr->last_child;

            if (rhs->type == AST_IDENTIFIER) {
                JSONArray edits{
                    serialize_insertion(cur_file, {point->ident->ident.ast_node->range.end + 1, "{"}),
                    serialize_insertion(cur_file, {point->top_level_dot_expr->ast_node->range.end, ", " + path + "}"}),
                };

                JSONObject result = params;
                result.add("additionalTextEdits", edits);
                return result;
            } else if (rhs->type == AST_USE_LIST) {
                TextPosition position;
                std::string text;

                unsigned num_children = rhs->num_children();
                unsigned num_commas = rhs->tokens.size() - 2;

                if (num_children == 0) {
                    position = cur_file.tokens.tokens[rhs->tokens[0]].position + 1;
                    text = path;
                } else if (num_children == num_commas) {
                    Token &trailing_comma = cur_file.tokens.tokens[rhs->tokens[rhs->tokens.size() - 2]];
                    position = trailing_comma.end();
                    text = " " + path + ",";
                } else {
                    position = rhs->last_child->range.end;
                    text = ", " + path;
                }

                JSONObject result = params;
                result.add("additionalTextEdits", JSONArray{serialize_insertion(cur_file, {position, text})});
                return result;
            }
        }
    }

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

    TextInsertion insertion;

    if (last_use) {
        insertion = insert_line_after(cur_file, last_use, use_text);
    } else {
        std::span<Token> attached_tokens = cur_file.tokens.get_attached_tokens(0);

        if (attached_tokens.size() == 0 || !attached_tokens[0].is(TKN_WHITESPACE) ||
            std::ranges::count(attached_tokens[0].value, '\n') == 0) {
            use_text += "\n";
        }

        insertion = {0, use_text};
    }

    JSONObject result = params;
    result.add("additionalTextEdits", JSONArray{serialize_insertion(cur_file, insertion)});
    return result;
}

std::optional<CompletionItemResolveHandler::UseInsertionPoint> CompletionItemResolveHandler::find_insertion_point(
    sir::Module &mod,
    sir::UseItem &use_item,
    lang::sir::UseDotExpr *top_level_dot_expr
) {
    if (auto ident = use_item.match<sir::UseIdent>()) {
        if (!top_level_dot_expr) {
            return {};
        }

        if (auto other_mod = ident->symbol.match<sir::Module>()) {
            unsigned depth = mod.path.num_common_ancestors(other_mod->path);

            if (depth > 0) {
                return UseInsertionPoint{
                    .top_level_dot_expr = top_level_dot_expr,
                    .ident = ident,
                    .common_ancestor = depth,
                };
            }
        }
    } else if (auto dot_expr = use_item.match<sir::UseDotExpr>()) {
        if (!top_level_dot_expr) {
            top_level_dot_expr = dot_expr;
        }

        auto lhs_point = find_insertion_point(mod, dot_expr->rhs, top_level_dot_expr);
        auto rhs_point = find_insertion_point(mod, dot_expr->lhs, top_level_dot_expr);

        if (lhs_point) {
            if (rhs_point) {
                return lhs_point->common_ancestor > rhs_point->common_ancestor ? lhs_point : rhs_point;
            } else {
                return lhs_point;
            }
        } else {
            return rhs_point;
        }
    } else if (auto list = use_item.match<sir::UseList>()) {
        std::optional<UseInsertionPoint> current;

        for (sir::UseItem item : list->items) {
            if (auto point = find_insertion_point(mod, item, nullptr)) {
                if (!current || point->common_ancestor > current->common_ancestor) {
                    current = point;
                }
            }
        }

        return current;
    }

    return {};
}

CompletionItemResolveHandler::TextInsertion CompletionItemResolveHandler::insert_line_after(
    SourceFile &file,
    ASTNode *node,
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

JSONObject CompletionItemResolveHandler::serialize_insertion(lang::SourceFile &file, TextInsertion insertion) {
    return JSONObject{
        {"range", ProtocolStructs::range_to_lsp(file.get_content(), {insertion.position, insertion.position})},
        {"newText", insertion.text},
    };
}

} // namespace banjo::lsp
