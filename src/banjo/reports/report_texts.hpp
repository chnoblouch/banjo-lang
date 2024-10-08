#ifndef REPORT_TEXTS_H
#define REPORT_TEXTS_H

#include "banjo/ast/ast_node.hpp"
#include "banjo/sir/sir.hpp"
#include "banjo/symbol/data_type.hpp"
#include "banjo/symbol/module_path.hpp"

#include <string>
#include <string_view>

namespace banjo {

namespace lang {

class ReportText {

public:
    enum ID {
        ERR_FIND_MODULE,

        ERR_PARSE_UNEXPECTED,
        ERR_PARSE_EXPECTED,
        ERR_PARSE_EXPECTED_SEMI,
        ERR_PARSE_EXPECTED_IDENTIFIER,
        ERR_PARSE_EXPECTED_TYPE,
        ERR_PARSE_UNCLOSED_BLOCK,

        ERR_EXPECTED_VALUE,
        ERR_REDEFINITION, // TODO: specialize for params
        ERR_NO_VALUE,
        ERR_NO_FUNCTION_OVERLOAD,
        ERR_NO_TYPE,
        ERR_NO_MEMBER,
        ERR_TYPE_NO_MEMBERS,
        ERR_FIELD_MISSING_TYPE,
        ERR_USE_NO_SYMBOL,
        ERR_INVALID_TYPE,
        ERR_TYPE_MISMATCH, // TODO: too generic
        ERR_RETURN_NO_VALUE,
        ERR_RETURN_TYPE_MISMATCH,
        ERR_NOT_ITERABLE,
        ERR_CANNOT_DEREFERENCE,
        ERR_INVALID_SELF_PARAM,
        ERR_SELF_NOT_FIRST_PARAM,
        ERR_INVALID_SELF,
        ERR_INT_LITERAL_TYPE,
        ERR_FLOAT_LITERAL_TYPE,
        ERR_CHAR_LITERAL_EMPTY,
        ERR_NEGATE_UNSIGNED,
        ERR_CANNOT_INFER_STRUCT_LITERAL_TYPE,
        ERR_USE_AFTER_MOVE,
        ERR_PRVATE,
        ERR_INVALID_TYPE_FIELD,
        ERR_INVALID_TYPE_METHOD,
        ERR_CANNOT_UNWRAP,
        ERR_NOT_PROTOCOL,
        ERR_PROTO_IMPL_MISSING_FUNC,
        ERR_SIGNATURE_MISMATCH,

        WARN_MISSING_FIELD,

        NOTE_USE_AFTER_MOVE_PREVIOUS,
        NOTE_USE_AFTER_MOVE_IN_LOOP,
    };

private:
    std::string text;

public:
    ReportText(ID id);
    ReportText(std::string_view format_str);
    std::string str() { return text; }

    ReportText &format(std::string_view string);
    ReportText &format(const char *string);
    ReportText &format(const std::string &string);
    ReportText &format(int integer);
    ReportText &format(unsigned integer);
    ReportText &format(long integer);
    ReportText &format(unsigned long integer);
    ReportText &format(long long integer);
    ReportText &format(unsigned long long integer);
    ReportText &format(ASTNode *node);
    ReportText &format(DataType *type);
    ReportText &format(Structure *struct_);
    ReportText &format(const ModulePath &path);
    ReportText &format(sir::Expr &expr);

public:
    static std::string to_string(const sir::Expr &expr);
};

} // namespace lang

} // namespace banjo

#endif
