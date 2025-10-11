#include "report_texts.hpp"

#include "banjo/sir/sir.hpp"
#include "banjo/sir/sir_visitor.hpp"

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

ReportText &ReportText::format(LargeInt integer) {
    return format(integer.to_string());
}

ReportText &ReportText::format(ASTNode *node) {
    return format(node->value);
}

ReportText &ReportText::format(const ModulePath &path) {
    return format(path.to_string());
}

ReportText &ReportText::format(sir::Expr &expr) {
    return format(to_string(expr));
}

ReportText &ReportText::format(sir::ExprCategory expr_category) {
    std::string string;

    switch (expr_category) {
        case sir::ExprCategory::VALUE: string = "value"; break;
        case sir::ExprCategory::TYPE: string = "type"; break;
        case sir::ExprCategory::VALUE_OR_TYPE: string = "value or type"; break;
        case sir::ExprCategory::MODULE: string = "module"; break;
        case sir::ExprCategory::META_ACCESS: string = "meta expression"; break;
    }

    return format(string);
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
        string += "'" + std::string{generic_params[i].ident.value} + "'";

        if (i != generic_params.size() - 1) {
            string += ", ";
        }
    }

    return format(string);
}

std::string ReportText::to_string(const sir::Expr &expr) {
    SIR_VISIT_EXPR(
        expr,
        return "invalid",
        return inner->value.to_string(),
        return std::to_string(inner->value),
        return inner->value ? "true" : "false",
        return "<char literal>",
        return "null",
        return "none",
        return "undefined",
        return "<array literal>",
        return "<string literal>",
        return "<struct literal>",
        return "<union case literal>",
        return "<map literal>",
        return "<closure literal>",
        return symbol_to_string(inner->symbol),
        return binary_expr_to_string(*inner),
        return "<unary expr>",
        return "<cast expr>",
        return "<index expr>",
        return "<call expr>",
        return "<field expr>",
        return "<range expr>",
        return tuple_expr_to_string(*inner),
        return "<coercion expr>",
        return primitive_to_string(inner->primitive),
        return "*" + to_string(inner->base_type),
        return "[" + to_string(inner->base_type) + "; " + to_string(inner->length) + "]",
        return func_type_to_string(*inner),
        return "<optional type>",
        return "<result type>",
        return "<array type>",
        return "<map type>",
        return closure_type_to_string(*inner),
        return reference_type_to_string(*inner),
        return "<ident expr>",
        return "<star expr>",
        return "<bracket expr>",
        return "<dot expr>",
        return pseudo_type_to_string(inner->kind),
        return "<meta access expr>",
        return "<meta field expr>",
        return "<meta call expr>",
        return "<init expr>",
        return "<move expr>",
        return "<deinit expr>",
        return "invalid"
    );
}

std::string ReportText::func_type_to_string(const sir::FuncType &func_type) {
    std::string str = "func(";

    for (unsigned i = 0; i < func_type.params.size(); i++) {
        str += to_string(func_type.params[i].type);

        if (i != func_type.params.size() - 1) {
            str += ", ";
        }
    }

    str += ")";

    if (!func_type.return_type.is_primitive_type(sir::Primitive::VOID)) {
        str += " -> " + to_string(func_type.return_type);
    }

    return str;
}

std::string ReportText::symbol_to_string(sir::Symbol symbol) {
    std::string str = symbol.get_name();

    if (auto struct_def = symbol.match<sir::StructDef>()) {
        if (struct_def->parent_specialization) {
            std::span<sir::Expr> generic_args = struct_def->parent_specialization->args;

            str += "[";

            for (unsigned i = 0; i < generic_args.size(); i++) {
                str += to_string(generic_args[i]);
                if (i != generic_args.size() - 1) {
                    str += ", ";
                }
            }

            str += "]";
        }
    }

    return str;
}

std::string ReportText::binary_expr_to_string(const sir::BinaryExpr &binary_expr) {
    std::string str = to_string(binary_expr.lhs) + " ";

    switch (binary_expr.op) {
        case sir::BinaryOp::ADD: str += "+"; break;
        case sir::BinaryOp::SUB: str += "-"; break;
        case sir::BinaryOp::MUL: str += "*"; break;
        case sir::BinaryOp::DIV: str += "/"; break;
        case sir::BinaryOp::MOD: str += "%"; break;
        case sir::BinaryOp::BIT_AND: str += "&"; break;
        case sir::BinaryOp::BIT_OR: str += "|"; break;
        case sir::BinaryOp::BIT_XOR: str += "^"; break;
        case sir::BinaryOp::SHL: str += "<<"; break;
        case sir::BinaryOp::SHR: str += ">>"; break;
        case sir::BinaryOp::EQ: str += "=="; break;
        case sir::BinaryOp::NE: str += "!="; break;
        case sir::BinaryOp::GT: str += ">"; break;
        case sir::BinaryOp::LT: str += "<"; break;
        case sir::BinaryOp::GE: str += ">="; break;
        case sir::BinaryOp::LE: str += "<="; break;
        case sir::BinaryOp::AND: str += "&&"; break;
        case sir::BinaryOp::OR: str += "||"; break;
    }

    str += " " + to_string(binary_expr.rhs);
    return str;
}

std::string ReportText::tuple_expr_to_string(const sir::TupleExpr &tuple_expr) {
    std::string str = "(";

    for (unsigned i = 0; i < tuple_expr.exprs.size(); i++) {
        str += to_string(tuple_expr.exprs[i]);

        if (i != tuple_expr.exprs.size() - 1) {
            str += ", ";
        }
    }

    str += ")";
    return str;
}

std::string ReportText::primitive_to_string(sir::Primitive primitive) {
    switch (primitive) {
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
}

std::string ReportText::closure_type_to_string(const sir::ClosureType &closure_type) {
    const sir::FuncType &func_type = closure_type.func_type;

    std::string str = "|";

    for (unsigned i = 0; i < func_type.params.size(); i++) {
        str += to_string(func_type.params[i].type);

        if (i != func_type.params.size() - 1) {
            str += ", ";
        }
    }

    str += "|";

    if (!func_type.return_type.is_primitive_type(sir::Primitive::VOID)) {
        str += " -> " + to_string(func_type.return_type);
    }

    return str;
}

std::string ReportText::reference_type_to_string(const sir::ReferenceType &reference_type) {
    if (reference_type.mut) {
        return "ref mut " + to_string(reference_type.base_type);
    } else {
        return "ref " + to_string(reference_type.base_type);
    }
}

std::string ReportText::pseudo_type_to_string(sir::PseudoTypeKind pseudo_type) {
    switch (pseudo_type) {
        case sir::PseudoTypeKind::INT_LITERAL: return "integer literal";
        case sir::PseudoTypeKind::FP_LITERAL: return "float literal";
        case sir::PseudoTypeKind::NULL_LITERAL: return "null";
        case sir::PseudoTypeKind::NONE_LITERAL: return "none";
        case sir::PseudoTypeKind::UNDEFINED_LITERAL: return "undefined";
        case sir::PseudoTypeKind::STRING_LITERAL: return "string literal";
        case sir::PseudoTypeKind::ARRAY_LITERAL: return "array literal";
        case sir::PseudoTypeKind::MAP_LITERAL: return "map literal";
    }
}

} // namespace lang

} // namespace banjo
