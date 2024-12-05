#ifndef BANJO_SEMA_COMPLETION_CONTEXT_H
#define BANJO_SEMA_COMPLETION_CONTEXT_H

#include "banjo/sir/sir.hpp"

#include <variant>

namespace banjo {

namespace lang {

namespace sema {

struct CompleteInDeclBlock {
    sir::DeclBlock *decl_block;
};

struct CompleteInBlock {
    sir::Block *block;
};

struct CompleteAfterDot {
    sir::Expr lhs;
};

struct CompleteInUse {};

struct CompleteAfterUseDot {
    sir::UseItem lhs;
};

struct CompleteInStructLiteral {
    sir::StructLiteral *struct_literal;
};

typedef std::variant<
    std::monostate,
    CompleteInDeclBlock,
    CompleteInBlock,
    CompleteAfterDot,
    CompleteInUse,
    CompleteAfterUseDot,
    CompleteInStructLiteral>
    CompletionContext;

} // namespace sema

} // namespace lang

} // namespace banjo

#endif
