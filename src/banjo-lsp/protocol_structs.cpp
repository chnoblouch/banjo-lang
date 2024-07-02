#include "protocol_structs.hpp"

#include "ast_navigation.hpp"

namespace banjo {

namespace lsp {

JSONObject ProtocolStructs::range_to_lsp(const std::string &source, lang::TextRange range) {
    LSPTextPosition start = ASTNavigation::pos_to_lsp(source, range.start);
    LSPTextPosition end = ASTNavigation::pos_to_lsp(source, range.end);

    return JSONObject{
        {"start", JSONObject{{"line", start.line}, {"character", start.column}}},
        {"end", JSONObject{{"line", end.line}, {"character", end.column}}}
    };
}

} // namespace lsp

} // namespace banjo
