#include "type_ssa_generator.hpp"

#include "banjo/sir/sir.hpp"
#include "banjo/utils/macros.hpp"

namespace banjo {

namespace lang {

TypeSSAGenerator::TypeSSAGenerator(SSAGeneratorContext &ctx) : ctx(ctx) {}

ssa::Type TypeSSAGenerator::generate(const sir::Expr &type) {
    if (auto primitive_type = type.match<sir::PrimitiveType>()) return generate_primitive_type(*primitive_type);
    else if (auto pointer_type = type.match<sir::PointerType>()) return ir::Primitive::ADDR;
    else if (auto func_type = type.match<sir::FuncType>()) return ir::Primitive::ADDR;
    else if (auto symbol_expr = type.match<sir::SymbolExpr>()) return generate_symbol_type(symbol_expr->symbol);
    else ASSERT_UNREACHABLE;
}

ssa::Type TypeSSAGenerator::generate_primitive_type(const sir::PrimitiveType &primitive_type) {
    switch (primitive_type.primitive) {
        case sir::Primitive::I8: return ssa::Primitive::I8;
        case sir::Primitive::I16: return ssa::Primitive::I16;
        case sir::Primitive::I32: return ssa::Primitive::I32;
        case sir::Primitive::I64: return ssa::Primitive::I64;
        case sir::Primitive::U8: return ssa::Primitive::I8;
        case sir::Primitive::U16: return ssa::Primitive::I16;
        case sir::Primitive::U32: return ssa::Primitive::I32;
        case sir::Primitive::U64: return ssa::Primitive::I64;
        case sir::Primitive::F32: return ssa::Primitive::F32;
        case sir::Primitive::F64: return ssa::Primitive::F64;
        case sir::Primitive::BOOL: return ssa::Primitive::I8;
        case sir::Primitive::ADDR: return ssa::Primitive::ADDR;
        case sir::Primitive::VOID: return ssa::Primitive::VOID;
    }
}

ssa::Type TypeSSAGenerator::generate_symbol_type(const sir::Symbol &symbol) {
    if (auto struct_ = symbol.match<sir::StructDef>()) return ctx.ssa_structs[struct_];
    else ASSERT_UNREACHABLE;
}

} // namespace lang

} // namespace banjo
