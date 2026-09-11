#include "sir_to_text.hpp"

namespace banjo::sir {

std::string_view to_text(BinaryOp binary_op) {
    switch (binary_op) {
        case sir::BinaryOp::ADD: return "+";
        case sir::BinaryOp::SUB: return "-";
        case sir::BinaryOp::MUL: return "*";
        case sir::BinaryOp::DIV: return "/";
        case sir::BinaryOp::MOD: return "%";
        case sir::BinaryOp::BIT_AND: return "&";
        case sir::BinaryOp::BIT_OR: return "|";
        case sir::BinaryOp::BIT_XOR: return "^";
        case sir::BinaryOp::SHL: return "<<";
        case sir::BinaryOp::SHR: return ">>";
        case sir::BinaryOp::EQ: return "==";
        case sir::BinaryOp::NE: return "!=";
        case sir::BinaryOp::GT: return ">";
        case sir::BinaryOp::LT: return "<";
        case sir::BinaryOp::GE: return ">=";
        case sir::BinaryOp::LE: return "<=";
        case sir::BinaryOp::AND: return "&&";
        case sir::BinaryOp::OR: return "||";
    }
}

std::string_view to_text(UnaryOp unary_op) {
    switch (unary_op) {
        case UnaryOp::NEG: return "-";
        case UnaryOp::BIT_NOT: return "~";
        case UnaryOp::ADDR: return "&";
        case UnaryOp::DEREF: return "*";
        case UnaryOp::NOT: return "!";
        case UnaryOp::REF: return "ref";
        case UnaryOp::REF_MUT: return "ref mut";
        case UnaryOp::SHARE: return "share";
    }
}

} // namespace banjo::sir
