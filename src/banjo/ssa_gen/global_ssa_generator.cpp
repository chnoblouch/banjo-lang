#include "global_ssa_generator.hpp"

#include "banjo/sir/sir.hpp"
#include "banjo/ssa_gen/ssa_generator_context.hpp"
#include "banjo/ssa_gen/type_ssa_generator.hpp"
#include "banjo/utils/macros.hpp"

namespace banjo {

namespace lang {

GlobalSSAGenerator::GlobalSSAGenerator(SSAGeneratorContext &ctx) : ctx(ctx) {}

ssa::Value GlobalSSAGenerator::generate_value(const sir::Expr &value) {
    if (auto int_literal = value.match<sir::IntLiteral>()) return generate_int_literal(*int_literal);
    else if (auto fp_literal = value.match<sir::FPLiteral>()) return generate_fp_literal(*fp_literal);
    else if (auto bool_literal = value.match<sir::BoolLiteral>()) return generate_bool_literal(*bool_literal);
    else if (auto char_literal = value.match<sir::CharLiteral>()) return generate_char_literal(*char_literal);
    else ASSERT_UNREACHABLE;
}

ssa::Value GlobalSSAGenerator::generate_int_literal(const sir::IntLiteral &int_literal) {
    ssa::Type ssa_type = TypeSSAGenerator(ctx).generate(int_literal.type);
    return ssa::Value::from_int_immediate(int_literal.value, ssa_type);
}

ssa::Value GlobalSSAGenerator::generate_fp_literal(const sir::FPLiteral &fp_literal) {
    ssa::Type ssa_type = TypeSSAGenerator(ctx).generate(fp_literal.type);
    return ssa::Value::from_fp_immediate(fp_literal.value, ssa_type);
}

ssa::Value GlobalSSAGenerator::generate_bool_literal(const sir::BoolLiteral &bool_literal) {
    ssa::Type ssa_type = TypeSSAGenerator(ctx).generate(bool_literal.type);
    return ssa::Value::from_int_immediate(static_cast<unsigned>(bool_literal.value), ssa_type);
}

ssa::Value GlobalSSAGenerator::generate_char_literal(const sir::CharLiteral &char_literal) {
    ssa::Type ssa_type = TypeSSAGenerator(ctx).generate(char_literal.type);
    return ssa::Value::from_int_immediate(static_cast<unsigned>(char_literal.value), ssa_type);
}

} // namespace lang

} // namespace banjo
