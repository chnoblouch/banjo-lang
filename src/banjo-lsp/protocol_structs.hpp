#ifndef BANJO_LSP_PROTOCOL_STRUCTS
#define BANJO_LSP_PROTOCOL_STRUCTS

#include "banjo/reports/report.hpp"
#include "banjo/source/text_range.hpp"
#include "banjo/utils/json.hpp"

#include <string_view>

namespace banjo {

namespace lsp {

enum SemanticTokenType {
    NAMESPACE,
    TYPE,
    CLASS,
    ENUM,
    INTERFACE,
    STRUCT,
    TYPE_PARAMETER,
    PARAMETER,
    VARIABLE,
    PROPERTY,
    ENUM_MEMBER,
    EVENT,
    FUNCTION,
    METHOD,
    MACRO,
    KEYWORD,
    MODIFIER,
    COMMENT,
    STRING,
    NUMBER,
    REGEXP,
    OPERATOR,
    DECORATOR
};

enum SemanticTokenModifiers {
    NONE = 0,
    DECLARATION = 1,
    DEFINITION = 2,
    READONLY = 4,
    STATIC = 8,
    DEPRECATED = 16,
    ABSTRACT = 32,
    ASYNC = 64,
    MODIFICATION = 128,
    DOCUMENTATION = 256,
    DEFAULT_LIBRARY = 512
};

enum DiagnosticSeverity {
    ERROR = 1,
    WARNING = 2,
    INFORMATION = 3,
    HINT = 4,
};

namespace ProtocolStructs {

JSONObject range_to_lsp(std::string_view source, lang::TextRange range);
DiagnosticSeverity report_type_to_lsp(lang::Report::Type type);

} // namespace ProtocolStructs

} // namespace lsp

} // namespace banjo

#endif
