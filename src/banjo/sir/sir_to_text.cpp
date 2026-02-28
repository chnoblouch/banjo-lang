#include "sir_to_text.hpp"

namespace banjo::lang::sir {

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

} // namespace banjo::lang::sir
