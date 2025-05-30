#ifndef TYPE_SSA_GENERATOR_H
#define TYPE_SSA_GENERATOR_H

#include "banjo/ssa/type.hpp"
#include "banjo/sir/sir.hpp"
#include "banjo/ssa_gen/ssa_generator_context.hpp"

namespace banjo {

namespace lang {

class TypeSSAGenerator {

private:
    SSAGeneratorContext &ctx;

public:
    TypeSSAGenerator(SSAGeneratorContext &ctx);
    ssa::Type generate(const sir::Expr &type);

private:
    ssa::Type generate_primitive_type(const sir::PrimitiveType &primitive_type);
    ssa::Type generate_symbol_type(const sir::SymbolExpr &symbol_type);
    ssa::Type generate_struct_type(const sir::StructDef &struct_def);
    ssa::Type generate_enum_type(const sir::EnumDef &enum_def);
    ssa::Type generate_union_type(const sir::UnionDef &union_def);
    ssa::Type generate_union_case_type(const sir::UnionCase &union_case);
    ssa::Type generate_pointer_type(const sir::PointerType &pointer_type);
    ssa::Type generate_tuple_type(const sir::TupleExpr &tuple_expr);
    ssa::Type generate_static_array_type(const sir::StaticArrayType &static_array_type);
    ssa::Type generate_func_type();
    ssa::Type generate_closure_type(const sir::ClosureType &closure_type);
    ssa::Type generate_reference_type();
};

} // namespace lang

} // namespace banjo

#endif
