#include "report_texts.hpp"

#include "banjo/reports/report_utils.hpp"
#include "banjo/sir/sir.hpp"
#include "banjo/utils/macros.hpp"

#include <map>

namespace banjo {

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
    {ReportText::ID::ERR_NOT_PROTOCOL, "'%' is not a protocol"},
    {ReportText::ID::ERR_PROTO_IMPL_MISSING_FUNC, "missing implementation of '%' from protocol '%'"},
    {ReportText::ID::ERR_SIGNATURE_MISMATCH, "signature of '%' does not match signature from protocol '%'"},

    {ReportText::ID::WARN_MISSING_FIELD, "missing field(s) in initializer of '%'"},

    {ReportText::ID::NOTE_USE_AFTER_MOVE_PREVIOUS, "previously moved here"},
    {ReportText::ID::NOTE_USE_AFTER_MOVE_IN_LOOP, "using this value inside a loop would move it in every iteration"},
};

ReportText::ReportText(ReportText::ID id) : text(TEXTS.at(id)) {}

ReportText::ReportText(std::string_view format_str) : text(format_str) {}

ReportText &ReportText::format(std::string_view string) {
#if BANJO_ENABLE_SIR
    std::string::size_type position = text.find('$');
#else
    std::string::size_type position = text.find('%');
#endif

    if (position != std::string::npos) {
        text.replace(position, 1, string);
    }

    return *this;
}

ReportText &ReportText::format(const char *string) {
    return format(std::string_view(string));
}

ReportText &ReportText::format(const std::string &string) {
    return format(std::string_view(string));
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

ReportText &ReportText::format(sir::Expr &expr) {
    return format(to_string(expr));
}

std::string ReportText::to_string(sir::Expr &expr) {
    if (!expr) {
        return "<null>";
    } else if (auto primitive_type = expr.match<sir::PrimitiveType>()) {
        switch (primitive_type->primitive) {
            case sir::Primitive::I8: return "i8";
            case sir::Primitive::I16: return "i16";
            case sir::Primitive::I32: return "i32";
            case sir::Primitive::I64: return "i64";
            case sir::Primitive::U8: return "u8";
            case sir::Primitive::U16: return "u16";
            case sir::Primitive::U32: return "u32";
            case sir::Primitive::U64: return "u64";
            case sir::Primitive::F32: return "f32";
            case sir::Primitive::F64: return "f64";
            case sir::Primitive::BOOL: return "bool";
            case sir::Primitive::ADDR: return "addr";
            case sir::Primitive::VOID: return "void";
        }
    } else if (auto pointer_type = expr.match<sir::PointerType>()) {
        return "*" + to_string(pointer_type->base_type);
    } else if (auto func_type = expr.match<sir::FuncType>()) {
        std::string params_str = "";

        for (unsigned i = 0; i < func_type->params.size(); i++) {
            params_str += to_string(func_type->params[i].type);
            if (i != func_type->params.size() - 1) {
                params_str += ", ";
            }
        }

        return "func(" + params_str + ") -> " + to_string(func_type->return_type);
    } else if (auto symbol = expr.match<sir::SymbolExpr>()) {
        return symbol->symbol.get_name();
    } else if (auto tuple_expr = expr.match<sir::TupleExpr>()) {
        std::string str = "(";

        for (unsigned i = 0; i < tuple_expr->exprs.size(); i++) {
            str += to_string(tuple_expr->exprs[i]);
            if (i != tuple_expr->exprs.size() - 1) {
                str += ", ";
            }
        }

        str += ")";
        return str;
    } else {
        return "<unknown>";
    }
}

} // namespace lang

} // namespace banjo
