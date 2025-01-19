#include "global_ssa_generator.hpp"

#include "banjo/sir/sir.hpp"
#include "banjo/ssa_gen/ssa_generator_context.hpp"
#include "banjo/ssa_gen/type_ssa_generator.hpp"
#include "banjo/utils/macros.hpp"

namespace banjo {

namespace lang {

GlobalSSAGenerator::GlobalSSAGenerator(SSAGeneratorContext &ctx) : ctx(ctx) {}

ssa::Global::Value GlobalSSAGenerator::generate_value(const sir::Expr &value) {
    if (auto int_literal = value.match<sir::IntLiteral>()) return generate_int_literal(*int_literal);
    else if (auto fp_literal = value.match<sir::FPLiteral>()) return generate_fp_literal(*fp_literal);
    else if (auto bool_literal = value.match<sir::BoolLiteral>()) return generate_bool_literal(*bool_literal);
    else if (auto char_literal = value.match<sir::CharLiteral>()) return generate_char_literal(*char_literal);
    else if (auto array_literal = value.match<sir::ArrayLiteral>()) return generate_array_literal(*array_literal);
    else if (auto tuple_literal = value.match<sir::TupleExpr>()) return generate_tuple_literal(*tuple_literal);
    else ASSERT_UNREACHABLE;
}

ssa::Global::Value GlobalSSAGenerator::generate_int_literal(const sir::IntLiteral &int_literal) {
    return int_literal.value;
}

ssa::Global::Value GlobalSSAGenerator::generate_fp_literal(const sir::FPLiteral &fp_literal) {
    return fp_literal.value;
}

ssa::Global::Value GlobalSSAGenerator::generate_bool_literal(const sir::BoolLiteral &bool_literal) {
    return static_cast<unsigned>(bool_literal.value);
}

ssa::Global::Value GlobalSSAGenerator::generate_char_literal(const sir::CharLiteral &char_literal) {
    return static_cast<unsigned>(char_literal.value);
}

ssa::Global::Value GlobalSSAGenerator::generate_array_literal(const sir::ArrayLiteral &array_literal) {
    if (array_literal.values.empty()) {
        return ssa::Global::None{};
    }

    ssa::Type type = TypeSSAGenerator(ctx).generate(array_literal.values[0].get_type());
    unsigned size = ctx.target->get_data_layout().get_size(type);
    unsigned alignment = ctx.target->get_data_layout().get_alignment(type);

    WriteBuffer buffer;

    for (sir::Expr value : array_literal.values) {
        generate_bytes(buffer, size, generate_value(value));
        pad_bytes(buffer, alignment);
    }

    return buffer.move_data();
}

ssa::Global::Value GlobalSSAGenerator::generate_tuple_literal(const sir::TupleExpr &tuple_literal) {
    if (tuple_literal.exprs.empty()) {
        return ssa::Global::None{};
    }

    WriteBuffer buffer;

    for (sir::Expr value : tuple_literal.exprs) {
        ssa::Type type = TypeSSAGenerator(ctx).generate(value.get_type());
        unsigned size = ctx.target->get_data_layout().get_size(type);

        pad_bytes(buffer, ctx.target->get_data_layout().get_alignment(type));
        generate_bytes(buffer, size, generate_value(value));
    }

    ssa::Type type = TypeSSAGenerator(ctx).generate(tuple_literal.type);
    pad_bytes(buffer, ctx.target->get_data_layout().get_alignment(type));

    return buffer.move_data();
}

void GlobalSSAGenerator::generate_bytes(WriteBuffer &buffer, unsigned size, const ssa::Global::Value &value) {
    if (auto int_value = std::get_if<ssa::Global::Integer>(&value)) {
        switch (size) {
            case 1: buffer.write_u8(int_value->to_bits()); break;
            case 2: buffer.write_u16(int_value->to_bits()); break;
            case 4: buffer.write_u32(int_value->to_bits()); break;
            case 8: buffer.write_u64(int_value->to_bits()); break;
            default: ASSERT_UNREACHABLE;
        }
    } else if (auto fp_value = std::get_if<ssa::Global::FloatingPoint>(&value)) {
        switch (size) {
            case 4: buffer.write_f32(*fp_value); break;
            case 8: buffer.write_f64(*fp_value); break;
            default: ASSERT_UNREACHABLE;
        }
    } else if (auto bytes = std::get_if<ssa::Global::Bytes>(&value)) {
        buffer.write_data(bytes->data(), bytes->size());
    } else {
        ASSERT_UNREACHABLE;
    }
}

void GlobalSSAGenerator::pad_bytes(WriteBuffer &buffer, unsigned alignment) {
    while (buffer.get_size() % alignment != 0) {
        buffer.write_u8(0);
    }
}

} // namespace lang

} // namespace banjo
