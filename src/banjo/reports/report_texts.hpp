#ifndef REPORT_TEXTS_H
#define REPORT_TEXTS_H

#include "ast/ast_node.hpp"
#include "symbol/data_type.hpp"
#include "symbol/module_path.hpp"

#include <string>

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

        WARN_MISSING_FIELD,

        NOTE_USE_AFTER_MOVE_PREVIOUS
    };

private:
    std::string text;

public:
    ReportText(ID id);
    ReportText(const char *format_str);
    std::string str() { return text; }

    ReportText &format(std::string string);
    ReportText &format(ASTNode *node);
    ReportText &format(DataType *type);
    ReportText &format(Structure *struct_);
    ReportText &format(const ModulePath &path);
};

} // namespace lang

#endif
