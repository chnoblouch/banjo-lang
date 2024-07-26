#ifndef EXPR_SSA_GENERATOR_H
#define EXPR_SSA_GENERATOR_H

#include "banjo/ir/comparison.hpp"
#include "banjo/ir/virtual_register.hpp"
#include "banjo/sir/sir.hpp"
#include "banjo/ssa_gen/ssa_generator_context.hpp"
#include "banjo/ssa_gen/storage_hints.hpp"
#include "banjo/ssa_gen/stored_value.hpp"

namespace banjo {

namespace lang {

class ExprSSAGenerator {

public:
    struct CondBranchTargets {
        ssa::BasicBlockIter target_if_true;
        ssa::BasicBlockIter target_if_false;
    };

private:
    SSAGeneratorContext &ctx;

public:
    ExprSSAGenerator(SSAGeneratorContext &ctx);

    StoredValue generate(const sir::Expr &expr);
    StoredValue generate_into_value(const sir::Expr &expr);
    void generate_into_dst(const sir::Expr &expr, const ssa::Value &dst);
    void generate_into_dst(const sir::Expr &expr, ssa::VirtualRegister dst);
    StoredValue generate(const sir::Expr &expr, const StorageHints &hints);
    void generate_branch(const sir::Expr &expr, CondBranchTargets targets);

private:
    StoredValue generate_int_literal(const sir::IntLiteral &int_literal);
    StoredValue generate_fp_literal(const sir::FPLiteral &fp_literal);
    StoredValue generate_bool_literal(const sir::BoolLiteral &bool_literal);
    StoredValue generate_char_literal(const sir::CharLiteral &char_literal);
    StoredValue generate_array_literal(const sir::ArrayLiteral &array_literal, const StorageHints &hints);
    StoredValue generate_string_literal(const sir::StringLiteral &string_literal);
    StoredValue generate_struct_literal(const sir::StructLiteral &struct_literal, const StorageHints &hints);
    StoredValue generate_symbol_expr(const sir::SymbolExpr &symbol_expr);
    StoredValue generate_binary_expr(const sir::BinaryExpr &binary_expr, const sir::Expr &expr);
    StoredValue generate_unary_expr(const sir::UnaryExpr &unary_expr, const sir::Expr &expr);
    StoredValue generate_neg(const sir::UnaryExpr &unary_expr);
    StoredValue generate_ref(const sir::UnaryExpr &unary_expr);
    StoredValue generate_deref(const sir::UnaryExpr &unary_expr);
    StoredValue generate_cast_expr(const sir::CastExpr &cast_expr);
    StoredValue generate_index_expr(const sir::IndexExpr &index_expr);
    StoredValue generate_call_expr(const sir::CallExpr &call_expr, const StorageHints &hints);
    StoredValue generate_field_expr(const sir::FieldExpr &field_expr);
    StoredValue generate_tuple_expr(const sir::TupleExpr &tuple_expr, const StorageHints &hints);

    StoredValue generate_bool_expr(const sir::Expr &expr);

    void generate_int_cmp_branch(
        const sir::BinaryExpr &binary_expr,
        ssa::Comparison ssa_cmp,
        CondBranchTargets targets
    );

    void generate_fp_cmp_branch(const sir::BinaryExpr &binary_expr, ssa::Comparison ssa_cmp, CondBranchTargets targets);
    void generate_and_branch(const sir::BinaryExpr &binary_expr, CondBranchTargets targets);
    void generate_or_branch(const sir::BinaryExpr &binary_expr, CondBranchTargets targets);
    void generate_not_branch(const sir::UnaryExpr &unary_expr, CondBranchTargets targets);
    void generate_zero_check_branch(const sir::Expr &expr, CondBranchTargets targets);
};

} // namespace lang

} // namespace banjo

#endif
