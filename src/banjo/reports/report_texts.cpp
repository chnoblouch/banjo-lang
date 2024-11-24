#include "report_texts.hpp"

#include "banjo/sir/sir.hpp"

#include <string>

namespace banjo {

namespace lang {

ReportText::ReportText(std::string_view format_str) : text(format_str) {}

ReportText &ReportText::format(std::string_view string) {
    std::string::size_type position = text.find('$');

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

ReportText &ReportText::format(int integer) {
    return format(std::to_string(integer));
}

ReportText &ReportText::format(unsigned integer) {
    return format(std::to_string(integer));
}

ReportText &ReportText::format(long integer) {
    return format(std::to_string(integer));
}

ReportText &ReportText::format(unsigned long integer) {
    return format(std::to_string(integer));
}

ReportText &ReportText::format(long long integer) {
    return format(std::to_string(integer));
}

ReportText &ReportText::format(unsigned long long integer) {
    return format(std::to_string(integer));
}

ReportText &ReportText::format(ASTNode *node) {
    return format(node->get_value());
}

ReportText &ReportText::format(const ModulePath &path) {
    return format(path.to_string("."));
}

ReportText &ReportText::format(sir::Expr &expr) {
    return format(to_string(expr));
}

ReportText &ReportText::format(const std::vector<sir::Expr> &exprs) {
    std::string string;

    for (unsigned i = 0; i < exprs.size(); i++) {
        string += to_string(exprs[i]);

        if (i != exprs.size() - 1) {
            string += ", ";
        }
    }

    return format(string);
}

ReportText &ReportText::format(const std::vector<sir::GenericParam> &generic_params) {
    std::string string;

    for (unsigned i = 0; i < generic_params.size(); i++) {
        string += "'" + generic_params[i].ident.value + "'";

        if (i != generic_params.size() - 1) {
            string += ", ";
        }
    }

    return format(string);
}

std::string ReportText::to_string(const sir::Expr &expr) {
    if (!expr) {
        return "<null>";
    } else if (auto int_literal = expr.match<sir::IntLiteral>()) {
        return int_literal->value.to_string();
    } else if (auto fp_literal = expr.match<sir::FPLiteral>()) {
        return std::to_string(fp_literal->value);
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
            case sir::Primitive::USIZE: return "usize";
            case sir::Primitive::F32: return "f32";
            case sir::Primitive::F64: return "f64";
            case sir::Primitive::BOOL: return "bool";
            case sir::Primitive::ADDR: return "addr";
            case sir::Primitive::VOID: return "void";
        }
    } else if (auto pointer_type = expr.match<sir::PointerType>()) {
        return "*" + to_string(pointer_type->base_type);
    } else if (auto static_array_type = expr.match<sir::StaticArrayType>()) {
        return "[" + to_string(static_array_type->base_type) + "; " + std::to_string(static_array_type->length) + "]";
    } else if (auto func_type = expr.match<sir::FuncType>()) {
        std::string params_str = "";

        for (unsigned i = 0; i < func_type->params.size(); i++) {
            params_str += to_string(func_type->params[i].type);
            if (i != func_type->params.size() - 1) {
                params_str += ", ";
            }
        }

        std::string str = "func(" + params_str + ")";

        if (!func_type->return_type.is_primitive_type(sir::Primitive::VOID)) {
            str += " -> " + to_string(func_type->return_type);
        }

        return str;
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
    } else if (auto pseudo_type = expr.match<sir::PseudoType>()) {
        switch (pseudo_type->kind) {
            case sir::PseudoTypeKind::INT_LITERAL: return "integer literal";
            case sir::PseudoTypeKind::FP_LITERAL: return "float literal";
            case sir::PseudoTypeKind::STRING_LITERAL: return "string literal";
            case sir::PseudoTypeKind::ARRAY_LITERAL: return "array literal";
            case sir::PseudoTypeKind::MAP_LITERAL: return "map literal";
        }
    } else {
        return "<unknown>";
    }
}

} // namespace lang

} // namespace banjo
