#include "type_ssa_generator.hpp"

#include "banjo/sir/sir.hpp"
#include "banjo/sir/sir_visitor.hpp"
#include "banjo/ssa/primitive.hpp"
#include "banjo/utils/macros.hpp"

namespace banjo {

namespace lang {

TypeSSAGenerator::TypeSSAGenerator(SSAGeneratorContext &ctx) : ctx(ctx) {}

ssa::Type TypeSSAGenerator::generate(const sir::Expr &type) {
    SIR_VISIT_TYPE(
        type,
        return ssa::Primitive::VOID,
        return generate_symbol_type(*inner),
        return generate_tuple_type(*inner),
        return generate_primitive_type(*inner),
        return generate_pointer_type(*inner),
        return generate_static_array_type(*inner),
        return generate_func_type(),
        return generate_closure_type(*inner),
        return generate_reference_type(),
        return ssa::Primitive::VOID
    );
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
        case sir::Primitive::USIZE: return ssa::Primitive::I64;
        case sir::Primitive::F32: return ssa::Primitive::F32;
        case sir::Primitive::F64: return ssa::Primitive::F64;
        case sir::Primitive::BOOL: return ssa::Primitive::I8;
        case sir::Primitive::ADDR: return ssa::Primitive::ADDR;
        case sir::Primitive::VOID: return ssa::Primitive::VOID;
    }
}

ssa::Type TypeSSAGenerator::generate_symbol_type(const sir::SymbolExpr &symbol_type) {
    const sir::Symbol &symbol = symbol_type.symbol;

    if (auto struct_def = symbol.match<sir::StructDef>()) return generate_struct_type(*struct_def);
    else if (auto enum_def = symbol.match<sir::EnumDef>()) return generate_enum_type(*enum_def);
    else if (auto union_def = symbol.match<sir::UnionDef>()) return generate_union_type(*union_def);
    else if (auto union_case = symbol.match<sir::UnionCase>()) return generate_union_case_type(*union_case);
    else ASSERT_UNREACHABLE;
}

ssa::Type TypeSSAGenerator::generate_struct_type(const sir::StructDef &struct_def) {
    return ctx.create_struct(struct_def);
}

ssa::Type TypeSSAGenerator::generate_enum_type(const sir::EnumDef & /*enum_def*/) {
    return ssa::Primitive::I32;
}

ssa::Type TypeSSAGenerator::generate_union_type(const sir::UnionDef &union_def) {
    return ctx.create_union(union_def);
}

ssa::Type TypeSSAGenerator::generate_union_case_type(const sir::UnionCase &union_case) {
    return ctx.create_union_case(union_case);
}

ssa::Type TypeSSAGenerator::generate_pointer_type(const sir::PointerType &pointer_type) {
    if (pointer_type.base_type.is_symbol<sir::ProtoDef>()) {
        return ctx.get_fat_pointer_type();
    } else {
        return ssa::Primitive::ADDR;
    }
}

ssa::Type TypeSSAGenerator::generate_tuple_type(const sir::TupleExpr &tuple_expr) {
    std::vector<ssa::Type> member_types(tuple_expr.exprs.size());

    for (unsigned i = 0; i < tuple_expr.exprs.size(); i++) {
        member_types[i] = generate(tuple_expr.exprs[i]);
    }

    return ctx.get_tuple_struct(member_types);
}

ssa::Type TypeSSAGenerator::generate_static_array_type(const sir::StaticArrayType &static_array_type) {
    unsigned length = static_array_type.length.as<sir::IntLiteral>().value.to_u64();

    ssa::Type type = generate(static_array_type.base_type);
    type.set_array_length(type.get_array_length() * length);
    return type;
}

ssa::Type TypeSSAGenerator::generate_func_type() {
    return ssa::Primitive::ADDR;
}

ssa::Type TypeSSAGenerator::generate_closure_type(const sir::ClosureType &closure_type) {
    return generate_struct_type(*closure_type.underlying_struct);
}

ssa::Type TypeSSAGenerator::generate_reference_type() {
    return ssa::Primitive::ADDR;
}

} // namespace lang

} // namespace banjo
