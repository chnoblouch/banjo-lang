#ifndef SIR_GENERATOR_H
#define SIR_GENERATOR_H

#include "banjo/ast/ast_module.hpp"
#include "banjo/ast/module_list.hpp"
#include "banjo/sir/sir.hpp"

#include <unordered_map>
#include <vector>

namespace banjo {

namespace lang {

class SIRGenerator {

private:
    struct Scope {
        sir::SymbolTable *symbol_table = nullptr;
        sir::StructDef *struct_def = nullptr;
    };

    sir::Unit sir_unit;
    sir::Module *cur_sir_mod;
    std::stack<Scope> scopes;
    std::unordered_map<ASTModule *, sir::Module *> mod_map;

public:
    sir::Unit generate(ModuleList &mods);

private:
    Scope &get_scope() { return scopes.top(); }

    Scope &push_scope() {
        scopes.push(scopes.top());
        return get_scope();
    }

    void pop_scope() { scopes.pop(); }

    sir::Module *generate_mod(ASTModule *node);

    sir::DeclBlock generate_decl_block(ASTNode *node);
    sir::Decl generate_decl(ASTNode *node);
    sir::Decl generate_func(ASTNode *node);
    sir::Decl generate_generic_func(ASTNode *node);
    sir::Decl generate_native_func(ASTNode *node);
    sir::Decl generate_struct(ASTNode *node);
    sir::Decl generate_enum(ASTNode *node);
    sir::Decl generate_var_decl(ASTNode *node);
    sir::Decl generate_use_decl(ASTNode *node);

    sir::Ident generate_ident(ASTNode *node);
    sir::FuncType generate_func_type(ASTNode *params_node, ASTNode *return_node);
    sir::Block generate_block(ASTNode *node);

    sir::Stmt generate_stmt(ASTNode *node);
    sir::Stmt generate_var_stmt(ASTNode *node);
    sir::Stmt generate_typeless_var_stmt(ASTNode *node);
    sir::Stmt generate_assign_stmt(ASTNode *node);
    sir::Stmt generate_comp_assign_stmt(ASTNode *node, sir::BinaryOp op);
    sir::Stmt generate_return_stmt(ASTNode *node);
    sir::Stmt generate_if_stmt(ASTNode *node);
    sir::Stmt generate_while_stmt(ASTNode *node);
    sir::Stmt generate_for_stmt(ASTNode *node);
    sir::Stmt generate_continue_stmt(ASTNode *node);
    sir::Stmt generate_break_stmt(ASTNode *node);

    sir::Expr generate_expr(ASTNode *node);
    sir::Expr generate_int_literal(ASTNode *node);
    sir::Expr generate_fp_literal(ASTNode *node);
    sir::Expr generate_bool_literal(ASTNode *node, bool value);
    sir::Expr generate_char_literal(ASTNode *node);
    sir::Expr generate_string_literal(ASTNode *node);
    sir::Expr generate_struct_literal(ASTNode *node);
    sir::Expr generate_ident_expr(ASTNode *node);
    sir::Expr generate_self(ASTNode *node);
    sir::Expr generate_binary_expr(ASTNode *node, sir::BinaryOp op);
    sir::Expr generate_unary_expr(ASTNode *node, sir::UnaryOp op);
    sir::Expr generate_cast_expr(ASTNode *node);
    sir::Expr generate_call_expr(ASTNode *node);
    sir::Expr generate_dot_expr(ASTNode *node);
    sir::Expr generate_range_expr(ASTNode *node);
    std::vector<sir::Expr> generate_arg_list(ASTNode *node);
    sir::Expr generate_star_expr(ASTNode *node);
    sir::Expr generate_bracket_expr(ASTNode *node);
    sir::Expr generate_primitive_type(ASTNode *node, sir::Primitive primitive);

    std::vector<sir::GenericParam> generate_generic_param_list(ASTNode *node);
    char decode_char(const std::string &value, unsigned &index);

    sir::UseItem generate_use_item(ASTNode *node);
    sir::UseItem generate_use_ident(ASTNode *node);
    sir::UseItem generate_use_rebind(ASTNode *node);
    sir::UseItem generate_use_dot_expr(ASTNode *node);
    sir::UseItem generate_use_list(ASTNode *node);

    template <typename T>
    T *create_expr(T value) {
        return cur_sir_mod->create_expr(value);
    }

    template <typename T>
    T *create_stmt(T value) {
        return cur_sir_mod->create_stmt(value);
    }

    template <typename T>
    T *create_decl(T value) {
        return cur_sir_mod->create_decl(value);
    }

    template <typename T>
    T *create_use_item(T value) {
        return cur_sir_mod->create_use_item(value);
    }

    sir::SymbolTable *create_symbol_table(sir::SymbolTable value) {
        return cur_sir_mod->create_symbol_table(std::move(value));
    }
};

} // namespace lang

} // namespace banjo

#endif
