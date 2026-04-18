#ifndef BANJO_SSA_GEN_SPECIALIZATION_COLLECTOR_H
#define BANJO_SSA_GEN_SPECIALIZATION_COLLECTOR_H

#include "banjo/sir/sir.hpp"
#include "banjo/utils/arena.hpp"

#include <unordered_map>
#include <vector>

namespace banjo::lang {

struct Specialization {};

class SpecializationCollector {

public:
    struct Entry {
        std::span<sir::GenericParam *> params;
        std::vector<sir::Expr> args;
    };

public:
    typedef std::unordered_map<sir::Symbol, std::vector<Entry>> Map;

private:
    utils::Arena<2048> &arena;
    Map specializations;
    std::vector<Entry> entry_stack;

public:
    SpecializationCollector(utils::Arena<2048> &arena);
    Map collect(const sir::Unit &unit);

private:
    void visit_decl_block(const sir::DeclBlock &decl_block);
    void visit_func_def(const sir::FuncDef &func_def, bool is_specialized = false);
    void visit_struct_def(const sir::StructDef &struct_def, bool is_specialized = false);

    void visit_block(const sir::Block &block);
    void visit_stmt(sir::Stmt stmt);
    void visit_var_stmt(const sir::VarStmt &var_stmt);
    void visit_assign_stmt(const sir::AssignStmt &assign_stmt);
    void visit_return_stmt(const sir::ReturnStmt &return_stmt);
    void visit_if_stmt(const sir::IfStmt &if_stmt);
    void visit_switch_stmt(const sir::SwitchStmt &switch_stmt);
    void visit_loop_stmt(const sir::LoopStmt &loop_stmt);

    void visit_expr(sir::Expr expr);
    void visit_struct_literal(const sir::StructLiteral &struct_literal);
    void visit_binary_expr(const sir::BinaryExpr &binary_expr);
    void visit_unary_expr(const sir::UnaryExpr &unary_expr);
    void visit_cast_expr(const sir::CastExpr &cast_expr);
    void visit_index_expr(const sir::IndexExpr &index_expr);
    void visit_call_expr(const sir::CallExpr &call_expr);
    void visit_field_expr(const sir::FieldExpr &field_expr);
    void visit_range_expr(const sir::RangeExpr &range_expr);
    void visit_try_expr(const sir::TryExpr &try_expr);
    void visit_tuple_expr(const sir::TupleExpr &tuple_expr);
    void visit_coercion_expr(const sir::CoercionExpr &coercion_expr);
    void visit_specialize_expr(const sir::SpecializeExpr &specialize_expr);
    void visit_pointer_type(const sir::PointerType &pointer_type);
    void visit_static_array_type(const sir::StaticArrayType &static_array_type);
    void visit_func_type(const sir::FuncType &func_type);
    void visit_reference_type(const sir::ReferenceType &reference_type);
    void visit_meta_access(const sir::MetaAccess &meta_access);
    void visit_meta_field_expr(const sir::MetaFieldExpr &meta_field_expr);
    void visit_meta_call_expr(const sir::MetaCallExpr &meta_call_expr);
    void visit_init_expr(const sir::InitExpr &init_expr);
    void visit_move_expr(const sir::MoveExpr &move_expr);
    void visit_deinit_expr(const sir::DeinitExpr &deinit_expr);
};

} // namespace banjo::lang

#endif
