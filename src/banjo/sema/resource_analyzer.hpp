#ifndef BANJO_SEMA_RESOURCE_ANALYZER_H
#define BANJO_SEMA_RESOURCE_ANALYZER_H

#include "banjo/sema/decl_visitor.hpp"
#include "banjo/sema/semantic_analyzer.hpp"
#include "banjo/sir/sir.hpp"

#include <stack>
#include <unordered_map>

namespace banjo {

namespace lang {

namespace sema {

class ResourceAnalyzer final : public DeclVisitor {

private:
    struct Scope;

    enum class InitState {
        INITIALIZED,
        UNINITIALIZED,
        COND_INITIALIZED,
    };

    struct MoveState {
        bool moved;
        bool conditional;
        bool partial;
        sir::Expr move_expr;
    };

    enum class ScopeType {
        GENERIC,
        LOOP,
    };

    struct Scope {
        ScopeType type;
        sir::Block *block;
        std::unordered_map<sir::Resource *, InitState> init_states;
        std::unordered_map<sir::Resource *, MoveState> move_states;
    };

    struct Context {
        bool moving;
        bool conditional;
        bool field_expr_lhs;
        bool in_resource_with_deinit;
        bool in_pointer;
        sir::Resource *cur_resource;
    };

    struct ResourceLocation {
        sir::Block *block = nullptr;
        sir::Resource *super_resource = nullptr;
    };

    std::vector<Scope> scopes;
    std::unordered_map<sir::Resource *, ResourceLocation> resource_locations;
    std::unordered_map<sir::Symbol, sir::Resource *> resources_by_symbols;

public:
    ResourceAnalyzer(SemanticAnalyzer &analyzer);
    Result analyze_func_def(sir::FuncDef &func_def) override;

    std::optional<sir::Resource> create_resource(sir::Expr type);

private:
    Scope analyze_block(sir::Block &block, ScopeType type = ScopeType::GENERIC);

    std::optional<sir::Resource> create_struct_resource(sir::StructDef &struct_def, sir::Expr type);
    std::optional<sir::Resource> create_tuple_resource(sir::TupleExpr &tuple_type, sir::Expr type);

    void insert_states(sir::Resource *resource, InitState init_state, ResourceLocation location);

    void analyze_var_stmt(sir::VarStmt &var_stmt);
    void analyze_assign_stmt(sir::AssignStmt &assign_stmt);
    void analyze_comp_assign_stmt(sir::CompAssignStmt &comp_assign_stmt);
    void analyze_return_stmt(sir::ReturnStmt &return_stmt);
    void analyze_if_stmt(sir::IfStmt &if_stmt);
    void analyze_try_stmt(sir::TryStmt &try_stmt);
    void analyze_loop_stmt(sir::LoopStmt &loop_stmt);
    void analyze_continue_stmt(sir::ContinueStmt &continue_stmt);
    void analyze_break_stmt(sir::BreakStmt &break_stmt);
    void analyze_block_stmt(sir::Block &block);

    Result analyze_expr(sir::Expr &expr, bool moving, bool conditional);
    Result analyze_expr(sir::Expr &expr, Context &ctx);
    Result analyze_array_literal(sir::ArrayLiteral &array_literal, Context &ctx);
    Result analyze_struct_literal(sir::StructLiteral &struct_literal, Context &ctx);
    Result analyze_unary_expr(sir::UnaryExpr &unary_expr, Context &ctx);
    Result analyze_symbol_expr(sir::SymbolExpr &symbol_expr, sir::Expr &out_expr, Context &ctx);
    Result analyze_call_expr(sir::CallExpr &call_expr, Context &ctx);
    Result analyze_field_expr(sir::FieldExpr &field_expr, sir::Expr &out_expr, Context &ctx);
    Result analyze_tuple_expr(sir::TupleExpr &tuple_expr, Context &ctx);
    Result analyze_deinit_expr(sir::DeinitExpr &deinit_expr, sir::Expr &out_expr);

    Result analyze_resource_use(sir::Resource *resource, sir::Expr &inout_expr, Context &ctx);
    MoveState *find_move_state(sir::Resource *resource);
    Result check_for_move_in_loop(sir::Resource *resource, sir::Expr move_expr);
    void move_sub_resources(sir::Resource *resource, sir::Expr move_expr, Context &ctx);
    void partially_move_super_resources(sir::Resource *resource, sir::Expr move_expr, Context &ctx);
    void update_init_state(Scope &scope, sir::Resource *resource, InitState value);
    void analyze_loop_jump();
    void mark_uninit_as_cond_init(Scope &scope);
    unsigned get_scope_depth();

    static bool is_resource(const sir::Expr &type);
    static void merge_move_states(Scope &parent_scope, Scope &child_scope, bool conditional);
};

} // namespace sema

} // namespace lang

} // namespace banjo

#endif
