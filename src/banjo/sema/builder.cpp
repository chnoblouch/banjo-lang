#include "builder.hpp"

#include "banjo/sema/semantic_analyzer.hpp"
#include "banjo/sir/sir.hpp"

namespace banjo::sema {

Builder::Builder(SemanticAnalyzer &analyzer) : analyzer{analyzer} {}

sir::UnaryExpr *Builder::create_deref_expr(sir::Expr value, sir::PointerType &type) {
    return analyzer.create<sir::UnaryExpr>({
        .ast_node = nullptr,
        .type = type.base_type,
        .op = sir::UnaryOp::DEREF,
        .value = value,
    });
}

sir::UnaryExpr *Builder::create_deref_expr(sir::Expr value, sir::ReferenceType &type) {
    return analyzer.create<sir::UnaryExpr>({
        .ast_node = nullptr,
        .type = type.base_type,
        .op = sir::UnaryOp::DEREF,
        .value = value,
    });
}

sir::UnaryExpr *Builder::create_ref_expr(sir::Expr value, bool mut) {
    sir::Expr type = analyzer.get_resolved_type(value);

    return analyzer.create<sir::UnaryExpr>({
        .ast_node = nullptr,
        .type = analyzer.create<sir::ReferenceType>({
            .ast_node = nullptr,
            .mut = mut,
            .base_type = type,
        }),
        .op = sir::UnaryOp::ADDR,
        .value = value,
    });
}

// sir::CallExpr *Builder::create_call_expr(sir::Concrete<sir::FuncDef> &concrete_func, std::span<sir::Expr> args) {}

sir::PrimitiveType *Builder::create_primitive_type(sir::Primitive primitive) {
    return analyzer.create<sir::PrimitiveType>({
        .ast_node = nullptr,
        .primitive = primitive,
    });
}

sir::PointerType *Builder::create_pointer_type(sir::Expr base_type) {
    return analyzer.create<sir::PointerType>({
        .ast_node = nullptr,
        .base_type = base_type,
    });
}

sir::ReferenceType *Builder::create_reference_type(sir::Expr base_type, bool mut) {
    return analyzer.create<sir::ReferenceType>({
        .ast_node = nullptr,
        .mut = mut,
        .base_type = base_type,
    });
}

sir::PseudoType *Builder::create_pseudo_type(sir::PseudoTypeKind kind) {
    return analyzer.create<sir::PseudoType>({
        .ast_node = nullptr,
        .kind = kind,
    });
}

sir::SymbolExpr *Builder::create_symbol_type(sir::Symbol symbol) {
    return analyzer.create<sir::SymbolExpr>({
        .ast_node = nullptr,
        .type = nullptr,
        .symbol = symbol,
    });
}

sir::CallExpr *Builder::create_method_call(
    sir::Expr base,
    sir::Concrete<sir::FuncDef> &concrete_func,
    ASTNode *ast_node
) {
    sir::Specializer specializer{analyzer.mod->trivial_arena, concrete_func};
    sir::FuncType *func_type = specializer.specialize_func_type(concrete_func.def->type);

    sir::SpecializeExpr *callee = analyzer.create<sir::SpecializeExpr>({
        .ast_node = nullptr,
        .type = func_type,
        .symbol = concrete_func.def,
        .args = concrete_func.generic_args,
    });

    return analyzer.create<sir::CallExpr>({
        .ast_node = ast_node,
        .type = func_type->return_type,
        .callee = callee,
        .args = analyzer.create_array<sir::Expr>({create_ref_expr(base, false)}),
    });
}

} // namespace banjo::sema
