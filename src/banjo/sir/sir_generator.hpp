#ifndef BANJO_SIR_GENERATOR_H
#define BANJO_SIR_GENERATOR_H

#include "banjo/ast/ast_module.hpp"
#include "banjo/ast/module_list.hpp"
#include "banjo/sir/sir.hpp"

#include <stack>
#include <string_view>
#include <vector>

namespace banjo {

namespace lang {

class SIRGenerator {

private:
    struct Scope {
        sir::SymbolTable *symbol_table = nullptr;
        sir::StructDef *struct_def = nullptr;
    };

    enum class MetaBlockKind {
        DECL,
        STMT,
    };

    sir::Module *cur_sir_mod;
    std::stack<Scope> scopes;

public:
    sir::Unit generate(ModuleList &mods);
    sir::Module generate(ASTModule *node);
    void regenerate_mod(sir::Unit &unit, ASTModule *mod);

private:
    void generate_mod(ASTModule *node, sir::Module &out_sir_mod);
    Scope &get_scope() { return scopes.top(); }

    Scope &push_scope() {
        scopes.push(scopes.top());
        return get_scope();
    }

    void pop_scope() { scopes.pop(); }

    sir::DeclBlock generate_decl_block(ASTNode *node);
    sir::Decl generate_decl(ASTNode *node);
    sir::Decl generate_func_def(ASTNode *node, sir::Attributes *attrs);
    sir::Decl generate_generic_func(ASTNode *node);
    sir::Decl generate_func_decl(ASTNode *node);
    sir::Decl generate_native_func(ASTNode *node, sir::Attributes *attrs);
    sir::Decl generate_const(ASTNode *node);
    sir::Decl generate_struct(ASTNode *node, sir::Attributes *attrs);
    sir::Decl generate_generic_struct(ASTNode *node);
    sir::Decl generate_var_decl(ASTNode *node, sir::Attributes *attrs);
    sir::Decl generate_native_var_decl(ASTNode *node, sir::Attributes *attrs);
    sir::Decl generate_enum(ASTNode *node);
    sir::Decl generate_union(ASTNode *node);
    sir::Decl generate_union_case(ASTNode *node);
    sir::Decl generate_proto(ASTNode *node);
    sir::Decl generate_type_alias(ASTNode *node);
    sir::Decl generate_use_decl(ASTNode *node);
    sir::Decl generate_error_decl(ASTNode *node);

    sir::Ident generate_ident(ASTNode *node);
    sir::Block generate_block(ASTNode *node);
    sir::MetaBlock generate_meta_block(ASTNode *node, MetaBlockKind kind);

    sir::Stmt generate_stmt(ASTNode *node);
    sir::Stmt generate_expr_stmt(ASTNode *node);
    sir::Stmt generate_var_stmt(ASTNode *node, sir::Attributes *attrs);
    sir::Stmt generate_typeless_var_stmt(ASTNode *node, sir::Attributes *attrs);
    sir::Stmt generate_ref_stmt(ASTNode *node, sir::Attributes *attrs, bool mut);
    sir::Stmt generate_typeless_ref_stmt(ASTNode *node, sir::Attributes *attrs, bool mut);
    sir::Stmt generate_assign_stmt(ASTNode *node);
    sir::Stmt generate_comp_assign_stmt(ASTNode *node, sir::BinaryOp op);
    sir::Stmt generate_return_stmt(ASTNode *node);
    sir::Stmt generate_if_stmt(ASTNode *node);
    sir::Stmt generate_switch_stmt(ASTNode *node);
    sir::Stmt generate_try_stmt(ASTNode *node);
    sir::Stmt generate_while_stmt(ASTNode *node);
    sir::Stmt generate_for_stmt(ASTNode *node);
    sir::Stmt generate_continue_stmt(ASTNode *node);
    sir::Stmt generate_break_stmt(ASTNode *node);
    sir::MetaIfStmt *generate_meta_if_stmt(ASTNode *node, MetaBlockKind kind);
    sir::MetaForStmt *generate_meta_for_stmt(ASTNode *node, MetaBlockKind kind);
    sir::Stmt generate_error_stmt(ASTNode *node);

    sir::Expr generate_expr(ASTNode *node);
    sir::Expr generate_int_literal(ASTNode *node);
    sir::Expr generate_fp_literal(ASTNode *node);
    sir::Expr generate_bool_literal(ASTNode *node, bool value);
    sir::Expr generate_char_literal(ASTNode *node);
    sir::Expr generate_null_literal(ASTNode *node);
    sir::Expr generate_none_literal(ASTNode *node);
    sir::Expr generate_undefined_literal(ASTNode *node);
    sir::Expr generate_array_literal(ASTNode *node);
    sir::Expr generate_string_literal(ASTNode *node);
    sir::Expr generate_struct_literal(ASTNode *node);
    sir::Expr generate_typeless_struct_literal(ASTNode *node);
    sir::Expr generate_map_literal(ASTNode *node);
    sir::Expr generate_closure_literal(ASTNode *node);
    sir::Expr generate_ident_expr(ASTNode *node);
    sir::Expr generate_self(ASTNode *node);
    sir::Expr generate_binary_expr(ASTNode *node, sir::BinaryOp op);
    sir::Expr generate_unary_expr(ASTNode *node, sir::UnaryOp op);
    sir::Expr generate_cast_expr(ASTNode *node);
    sir::Expr generate_call_expr(ASTNode *node);
    sir::Expr generate_dot_expr(ASTNode *node);
    sir::Expr generate_implicit_dot_expr(ASTNode *node);
    sir::Expr generate_range_expr(ASTNode *node);
    sir::Expr generate_tuple_expr(ASTNode *node);
    sir::Expr generate_star_expr(ASTNode *node);
    sir::Expr generate_bracket_expr(ASTNode *node);
    sir::Expr generate_primitive_type(ASTNode *node, sir::Primitive primitive);
    sir::Expr generate_static_array_type(ASTNode *node);
    sir::Expr generate_func_type(ASTNode *node);
    sir::Expr generate_optional_type(ASTNode *node);
    sir::Expr generate_result_type(ASTNode *node);
    sir::Expr generate_closure_type(ASTNode *node);
    sir::Expr generate_meta_access(ASTNode *node);
    sir::Expr generate_error_expr(ASTNode *node);

    sir::FuncType generate_func_type(ASTNode *params_node, ASTNode *return_node);
    sir::Param generate_param(ASTNode *node);
    std::vector<sir::GenericParam> generate_generic_param_list(ASTNode *node);
    sir::Local generate_local(ASTNode *ident_node, ASTNode *type_node, sir::Attributes *attrs);
    std::vector<sir::Expr> generate_expr_list(ASTNode *node);
    std::span<sir::Expr> generate_expr_span(ASTNode *node);
    std::span<sir::StructLiteralEntry> generate_struct_literal_entries(ASTNode *node);
    std::span<sir::MapLiteralEntry> generate_map_literal_entries(ASTNode *node);
    std::vector<sir::UnionCaseField> generate_union_case_fields(ASTNode *node);
    sir::Attributes *generate_attrs(ASTNode *node);
    char decode_char(std::string_view value, unsigned &index);

    sir::UseItem generate_use_item(ASTNode *node);
    sir::UseItem generate_use_ident(ASTNode *node);
    sir::UseItem generate_use_rebind(ASTNode *node);
    sir::UseItem generate_use_dot_expr(ASTNode *node);
    sir::UseItem generate_use_list(ASTNode *node);
    sir::UseItem generate_use_completion_token(ASTNode *node);
    sir::UseItem generate_error_use_item(ASTNode *node);

    template <typename T>
    T *create(T value) {
        return cur_sir_mod->create(value);
    }

    template <typename T>
    std::span<T> allocate_array(unsigned length) {
        return cur_sir_mod->allocate_array<T>(length);
    }

    template <typename T>
    std::span<T> create_array(std::span<T> values) {
        return cur_sir_mod->create_array(std::move(values));
    }

    template <typename T>
    std::span<T> create_array(std::initializer_list<T> values) {
        return cur_sir_mod->create_array(std::move(values));
    }

    std::string_view create_string(std::string_view value) { return cur_sir_mod->create_string(value); }
    sir::IdentExpr *generate_completion_token(ASTNode *node);
};

} // namespace lang

} // namespace banjo

#endif
