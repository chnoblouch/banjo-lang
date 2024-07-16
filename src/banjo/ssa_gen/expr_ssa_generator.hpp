#ifndef EXPR_SSA_GENERATOR_H
#define EXPR_SSA_GENERATOR_H

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
    void generate_bool_expr(const sir::Expr &expr, CondBranchTargets branch_targets);

private:
    StoredValue generate_int_literal(const sir::IntLiteral &int_literal);
    StoredValue generate_char_literal(const sir::CharLiteral &char_literal);
    StoredValue generate_string_literal(const sir::StringLiteral &string_literal);
    StoredValue generate_struct_literal(const sir::StructLiteral &struct_literal, const StorageHints &hints);
    StoredValue generate_ident_expr(const sir::IdentExpr &ident_expr);
    StoredValue generate_binary_expr(const sir::BinaryExpr &binary_expr);
    StoredValue generate_unary_expr(const sir::UnaryExpr &unary_expr);
    StoredValue generate_ref(const sir::UnaryExpr &unary_expr);
    StoredValue generate_deref(const sir::UnaryExpr &unary_expr);
    StoredValue generate_call_expr(const sir::CallExpr &call_expr, const StorageHints &hints);
    StoredValue generate_dot_expr(const sir::DotExpr &dot_expr);

    void generate_comparison(const sir::BinaryExpr &binary_expr, CondBranchTargets branch_targets);
};

} // namespace lang

} // namespace banjo

#endif
