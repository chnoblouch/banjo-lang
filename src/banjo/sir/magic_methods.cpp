#include "magic_methods.hpp"

#include "banjo/sir/magic_methods.hpp"
#include "banjo/sir/sir.hpp"
#include "banjo/utils/macros.hpp"

namespace banjo {

namespace lang {

namespace sir {

namespace MagicMethods {

std::string_view look_up(sir::BinaryOp op) {
    switch (op) {
        case sir::BinaryOp::ADD: return OP_ADD;
        case sir::BinaryOp::SUB: return OP_SUB;
        case sir::BinaryOp::MUL: return OP_MUL;
        case sir::BinaryOp::DIV: return OP_DIV;
        case sir::BinaryOp::MOD: return OP_MOD;
        case sir::BinaryOp::BIT_AND: return OP_BIT_AND;
        case sir::BinaryOp::BIT_OR: return OP_BIT_OR;
        case sir::BinaryOp::BIT_XOR: return OP_BIT_XOR;
        case sir::BinaryOp::SHL: return OP_SHL;
        case sir::BinaryOp::SHR: return OP_SHR;
        case sir::BinaryOp::EQ: return OP_EQ;
        case sir::BinaryOp::NE: return OP_NE;
        case sir::BinaryOp::GT: return OP_GT;
        case sir::BinaryOp::LT: return OP_LT;
        case sir::BinaryOp::GE: return OP_GE;
        case sir::BinaryOp::LE: return OP_LE;
        case sir::BinaryOp::AND: ASSERT_UNREACHABLE;
        case sir::BinaryOp::OR: ASSERT_UNREACHABLE;
    }
}

std::string_view look_up(sir::UnaryOp op) {
    switch (op) {
        case sir::UnaryOp::NEG: return OP_NEG;
        case sir::UnaryOp::BIT_NOT: return OP_BIT_NOT;
        case sir::UnaryOp::REF: ASSERT_UNREACHABLE;
        case sir::UnaryOp::DEREF: return OP_DEREF;
        case sir::UnaryOp::NOT: ASSERT_UNREACHABLE;
    }
}

std::string_view look_up_iter(sir::IterKind kind) {
    switch (kind) {
        case sir::IterKind::MOVE: return ITER;
        case sir::IterKind::REF: return REF_ITER;
        case sir::IterKind::MUT: return MUT_ITER;
    }
}

} // namespace MagicMethods

} // namespace sir

} // namespace lang

} // namespace banjo
