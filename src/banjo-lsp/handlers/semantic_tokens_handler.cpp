#include "semantic_tokens_handler.hpp"

#include "banjo/sir/sir.hpp"
#include "banjo/source/text_range.hpp"
#include "protocol_structs.hpp"
#include "uri.hpp"

#include <algorithm>

namespace banjo {

namespace lsp {

using namespace lang;

SemanticTokensHandler::SemanticTokensHandler(Workspace &workspace) : workspace(workspace) {}

SemanticTokensHandler::~SemanticTokensHandler() {}

JSONValue SemanticTokensHandler::handle(const JSONObject &params, Connection &connection) {
    std::string uri = params.get_object("textDocument").get_string("uri");
    std::filesystem::path fs_path = URI::decode_to_path(uri);

    lang::SourceFile *file = workspace.find_file(fs_path);
    if (!file) {
        return JSONObject{{"data", JSONArray{}}};
    }

    ModuleIndex *index = workspace.find_index(file->sir_mod);
    if (!index) {
        return JSONObject{{"data", JSONArray{}}};
    }

    std::vector<SemanticToken> tokens;

    for (const SymbolRef &symbol_ref : index->symbol_refs) {
        add_symbol_token(tokens, symbol_ref.range, symbol_ref.symbol);
    }

    std::sort(tokens.begin(), tokens.end(), [](const SemanticToken &lhs, const SemanticToken &rhs) {
        return lhs.range.start < rhs.range.start;
    });

    std::vector<LSPSemanticToken> lsp_tokens = tokens_to_lsp(file->buffer, tokens);
    JSONArray data = serialize(lsp_tokens);

    return JSONObject{{"data", data}};
}

std::vector<LSPSemanticToken> SemanticTokensHandler::tokens_to_lsp(
    const std::string &source,
    const std::vector<SemanticToken> &tokens
) {
    std::vector<LSPSemanticToken> lsp_tokens;
    lang::TextPosition position = 0;

    for (const SemanticToken &token : tokens) {
        int delta_line = 0;
        int delta_start_column = 0;

        while (position < token.range.start) {
            if (source[position] == '\n') {
                delta_line += 1;
                delta_start_column = 0;
            } else {
                delta_start_column += 1;
            }

            position += 1;
        }

        lsp_tokens.push_back({
            .delta_line = delta_line,
            .delta_start_column = delta_start_column,
            .length = static_cast<int>(token.range.end - token.range.start),
            .type = token.type,
            .modifiers = token.modifiers,
        });
    }

    return lsp_tokens;
}

JSONArray SemanticTokensHandler::serialize(const std::vector<LSPSemanticToken> &lsp_tokens) {
    JSONArray data;

    for (const LSPSemanticToken &lsp_token : lsp_tokens) {
        data.add(lsp_token.delta_line);
        data.add(lsp_token.delta_start_column);
        data.add(lsp_token.length);
        data.add(lsp_token.type);
        data.add(lsp_token.modifiers);
    }

    return data;
}

void SemanticTokensHandler::add_symbol_token(
    std::vector<SemanticToken> &tokens,
    TextRange range,
    const sir::Symbol &symbol
) {
    if (symbol.is<sir::Module>()) {
        tokens.push_back({range, SemanticTokenType::NAMESPACE, SemanticTokenModifiers::NONE});
    } else if (symbol.is_one_of<sir::FuncDef, sir::FuncDecl, sir::NativeFuncDecl>()) {
        tokens.push_back({range, SemanticTokenType::FUNCTION, SemanticTokenModifiers::NONE});
    } else if (symbol.is<sir::ConstDef>()) {
        tokens.push_back({range, SemanticTokenType::VARIABLE, SemanticTokenModifiers::READONLY});
    } else if (symbol.is_one_of<
                   sir::StructDef,
                   sir::EnumDef,
                   sir::UnionDef,
                   sir::UnionCase,
                   sir::ProtoDef,
                   sir::TypeAlias>()) {
        tokens.push_back({range, SemanticTokenType::STRUCT, SemanticTokenModifiers::NONE});
    } else if (symbol.is<sir::StructField>()) {
        tokens.push_back({range, SemanticTokenType::PROPERTY, SemanticTokenModifiers::NONE});
    } else if (symbol.is_one_of<sir::VarDecl, sir::NativeVarDecl, sir::Local>()) {
        tokens.push_back({range, SemanticTokenType::VARIABLE, SemanticTokenModifiers::NONE});
    } else if (symbol.is<sir::EnumVariant>()) {
        tokens.push_back({range, SemanticTokenType::ENUM_MEMBER, SemanticTokenModifiers::NONE});
    } else if (symbol.is<sir::Param>()) {
        tokens.push_back({range, SemanticTokenType::PARAMETER, SemanticTokenModifiers::NONE});
    } else if (symbol.is<sir::GenericParam>()) {
        tokens.push_back({range, SemanticTokenType::TYPE_PARAMETER, SemanticTokenModifiers::NONE});
    }
}

} // namespace lsp

} // namespace banjo
