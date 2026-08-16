#include "protocol_structs.hpp"

#include "ast_navigation.hpp"

namespace banjo::lsp {

json::Object ProtocolStructs::range_to_lsp(std::string_view source, TextRange range) {
    LSPTextPosition start = ASTNavigation::pos_to_lsp(source, range.start);
    LSPTextPosition end = ASTNavigation::pos_to_lsp(source, range.end);

    return json::Object{
        {"start", json::Object{{"line", start.line}, {"character", start.column}}},
        {"end", json::Object{{"line", end.line}, {"character", end.column}}}
    };
}

DiagnosticSeverity ProtocolStructs::report_type_to_lsp(Report::Type type) {
    switch (type) {
        case Report::Type::ERROR: return DiagnosticSeverity::ERROR;
        case Report::Type::WARNING: return DiagnosticSeverity::WARNING;
    }
}

} // namespace banjo::lsp
