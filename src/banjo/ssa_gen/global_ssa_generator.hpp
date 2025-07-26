#ifndef BANJO_SSA_GENERATOR_GLOBAL_SSA_GENERATOR_H
#define BANJO_SSA_GENERATOR_GLOBAL_SSA_GENERATOR_H

#include "banjo/sir/sir.hpp"
#include "banjo/ssa_gen/ssa_generator_context.hpp"
#include "banjo/utils/write_buffer.hpp"

namespace banjo {

namespace lang {

class GlobalSSAGenerator {

private:
    SSAGeneratorContext &ctx;

public:
    GlobalSSAGenerator(SSAGeneratorContext &ctx);
    ssa::Global::Value generate_value(const sir::Expr &value);

private:
    ssa::Global::Value generate_int_literal(const sir::IntLiteral &int_literal);
    ssa::Global::Value generate_fp_literal(const sir::FPLiteral &fp_literal);
    ssa::Global::Value generate_bool_literal(const sir::BoolLiteral &bool_literal);
    ssa::Global::Value generate_char_literal(const sir::CharLiteral &char_literal);
    ssa::Global::Value generate_array_literal(const sir::ArrayLiteral &array_literal);
    ssa::Global::Value generate_tuple_literal(const sir::TupleExpr &tuple_literal);

    void generate_bytes(WriteBuffer &buffer, unsigned size, const ssa::Global::Value &value);
    void pad_bytes(WriteBuffer &buffer, unsigned alignment);
};

} // namespace lang

} // namespace banjo

#endif
