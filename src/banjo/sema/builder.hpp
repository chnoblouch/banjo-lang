#ifndef BANJO_SEMA_BUILDER_H
#define BANJO_SEMA_BUILDER_H

#include "banjo/sir/sir.hpp"

namespace banjo::sema {

class SemanticAnalyzer;

class Builder {

private:
    SemanticAnalyzer &analyzer;

public:
    Builder(SemanticAnalyzer &analyzer);

    sir::UnaryExpr *create_ref_expr(sir::Expr value, bool mut);
    sir::CallExpr *create_call_expr(sir::Concrete<sir::FuncDef> &concrete_func, std::span<sir::Expr> args);

    sir::PrimitiveType *create_primitive_type(sir::Primitive primitive);
    sir::PointerType *create_pointer_type(sir::Expr base_type);
    sir::ReferenceType *create_reference_type(sir::Expr base_type, bool mut);
    sir::PseudoType *create_pseudo_type(sir::PseudoTypeKind kind);
};

} // namespace banjo::sema

#endif
