#include "report_texts.hpp"

#include "reports/report_utils.hpp"
#include <map>

namespace lang {

const std::map<ReportText::ID, std::string> TEXTS = {
    {ReportText::ID::ERR_FIND_MODULE, "failed to find module"},

    {ReportText::ID::ERR_PARSE_UNEXPECTED, "unexpected token %"},
    {ReportText::ID::ERR_PARSE_EXPECTED, "expected %, got %"},
    {ReportText::ID::ERR_PARSE_EXPECTED_SEMI, "expected ';' after statement"},
    {ReportText::ID::ERR_PARSE_EXPECTED_IDENTIFIER, "expected identifier, got %"},
    {ReportText::ID::ERR_PARSE_EXPECTED_TYPE, "expected type, got %"},
    {ReportText::ID::ERR_PARSE_UNCLOSED_BLOCK, "file ends with unclosed block"},

    {ReportText::ID::ERR_EXPECTED_VALUE, "expected value"},
    {ReportText::ID::ERR_REDEFINITION, "redefinition of '%'"},
    {ReportText::ID::ERR_NO_VALUE, "cannot find '%' in this scope"},
    {ReportText::ID::ERR_NO_FUNCTION_OVERLOAD, "no matching function overload for '%'"},
    {ReportText::ID::ERR_NO_TYPE, "cannot find type '%' in this scope"},
    {ReportText::ID::ERR_NO_MEMBER, "no member named '%' in '%'"},
    {ReportText::ID::ERR_TYPE_NO_MEMBERS, "type '%' has no members"},
    {ReportText::ID::ERR_FIELD_MISSING_TYPE, "missing type ascription for field '%'"},
    {ReportText::ID::ERR_USE_NO_SYMBOL, "used symbol was not found"},
    {ReportText::ID::ERR_INVALID_TYPE, "invalid type"},
    {ReportText::ID::ERR_TYPE_MISMATCH, "type mismatch (expected '%', got '%')"},
    {ReportText::ID::ERR_RETURN_NO_VALUE, "missing value in return"},
    {ReportText::ID::ERR_RETURN_TYPE_MISMATCH, "value type '%' does not match function return type '%'"},
    {ReportText::ID::ERR_NOT_ITERABLE, "value of type '%' is not iterable"},
    {ReportText::ID::ERR_CANNOT_DEREFERENCE, "type '%' cannot be dereferenced"},
    {ReportText::ID::ERR_INVALID_SELF_PARAM, "'self' parameter is only allowed in methods"},
    {ReportText::ID::ERR_SELF_NOT_FIRST_PARAM, "'self' must be the first parameter"},
    {ReportText::ID::ERR_INVALID_SELF, "'self' is only allowed in methods"},
    {ReportText::ID::ERR_INT_LITERAL_TYPE, "cannot convert integer literal to type '%'"},
    {ReportText::ID::ERR_FLOAT_LITERAL_TYPE, "cannot convert floating-point literal to type '%'"},
    {ReportText::ID::ERR_CHAR_LITERAL_EMPTY, "empty character literal"},
    {ReportText::ID::ERR_NEGATE_UNSIGNED, "cannot negate value having unsigned type '%'"},
    {ReportText::ID::ERR_CANNOT_INFER_STRUCT_LITERAL_TYPE, "cannot infer struct literal type"},
    {ReportText::ID::ERR_USE_AFTER_MOVE, "value used after move"},
    {ReportText::ID::ERR_PRVATE, "cannot access private symbol '%'"},
    {ReportText::ID::ERR_INVALID_TYPE_FIELD, "invalid type field '%'"},
    {ReportText::ID::ERR_INVALID_TYPE_METHOD, "invalid type method '%'"},
    {ReportText::ID::ERR_CANNOT_UNWRAP, "cannot unwrap value of type '%'"},

    {ReportText::ID::WARN_MISSING_FIELD, "missing field(s) in initializer of '%'"},

    {ReportText::ID::NOTE_USE_AFTER_MOVE_PREVIOUS, "previously moved here"}
};

ReportText::ReportText(ReportText::ID id) : text(TEXTS.at(id)) {}

ReportText::ReportText(const char *format_str) : text(format_str) {}

ReportText &ReportText::format(std::string string) {
    std::string::size_type position = text.find('%');
    if (position != std::string::npos) {
        text.replace(position, 1, string);
    }

    return *this;
}

ReportText &ReportText::format(ASTNode *node) {
    return format(node->get_value());
}

ReportText &ReportText::format(DataType *type) {
    return format(ReportUtils::type_to_string(type));
}

ReportText &ReportText::format(Structure *struct_) {
    return format(ReportUtils::struct_to_string(struct_));
}

ReportText &ReportText::format(const ModulePath &path) {
    return format(path.to_string("."));
}

} // namespace lang
