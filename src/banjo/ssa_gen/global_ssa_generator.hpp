#ifndef GLOBAL_SSA_GENERATOR_H
#define GLOBAL_SSA_GENERATOR_H

#include "banjo/sir/sir.hpp"
#include "banjo/ssa_gen/ssa_generator_context.hpp"

namespace banjo {

namespace lang {

class GlobalSSAGenerator {

private:
    SSAGeneratorContext &ctx;

public:
    GlobalSSAGenerator(SSAGeneratorContext &ctx);
    ssa::Value generate_value(const sir::Expr &value);

private:
    ssa::Value generate_int_literal(const sir::IntLiteral &int_literal);
    ssa::Value generate_fp_literal(const sir::FPLiteral &fp_literal);
    ssa::Value generate_bool_literal(const sir::BoolLiteral &bool_literal);
    ssa::Value generate_char_literal(const sir::CharLiteral &char_literal);
};

} // namespace lang

} // namespace banjo

#endif
