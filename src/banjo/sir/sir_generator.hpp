#ifndef SIR_GENERATOR_H
#define SIR_GENERATOR_H

#include "banjo/ast/ast_module.hpp"
#include "banjo/sir/sir.hpp"

#include <vector>

namespace banjo {

namespace lang {

class SIRGenerator {

private:
    struct Scope {
        sir::SymbolTable *symbol_table = nullptr;
    };

    sir::Unit sir_unit;
    std::stack<Scope> scopes;

public:
    sir::Unit generate(ASTModule *mod);

private:
    Scope &get_scope() { return scopes.top(); }

    Scope &push_scope() {
        scopes.push(scopes.top());
        return get_scope();
    }

    void pop_scope() { scopes.pop(); }

    sir::DeclBlock generate_decl_block(ASTNode *node);
    sir::Decl generate_decl(ASTNode *node);
    sir::Decl generate_func(ASTNode *node);
    sir::Decl generate_native_func(ASTNode *node);
    sir::Decl generate_struct(ASTNode *node);
    sir::Decl generate_var_decl(ASTNode *node);

    sir::Ident generate_ident(ASTNode *node);
    sir::FuncType generate_func_type(ASTNode *params_node, ASTNode *return_node);
    sir::Block generate_block(ASTNode *node);

    sir::Stmt generate_stmt(ASTNode *node);
    sir::Stmt generate_var_stmt(ASTNode *node);
    sir::Stmt generate_typeless_var_stmt(ASTNode *node);
    sir::Stmt generate_assign_stmt(ASTNode *node);
    sir::Stmt generate_return_stmt(ASTNode *node);
    sir::Stmt generate_if_stmt(ASTNode *node);

    sir::Expr generate_expr(ASTNode *node);
    sir::Expr generate_int_literal(ASTNode *node);
    sir::Expr generate_fp_literal(ASTNode *node);
    sir::Expr generate_bool_literal(ASTNode *node, bool value);
    sir::Expr generate_char_literal(ASTNode *node);
    sir::Expr generate_string_literal(ASTNode *node);
    sir::Expr generate_struct_literal(ASTNode *node);
    sir::Expr generate_ident_expr(ASTNode *node);
    sir::Expr generate_binary_expr(ASTNode *node, sir::BinaryOp op);
    sir::Expr generate_unary_expr(ASTNode *node, sir::UnaryOp op);
    sir::Expr generate_cast_expr(ASTNode *node);
    sir::Expr generate_call_expr(ASTNode *node);
    sir::Expr generate_dot_expr(ASTNode *node);
    std::vector<sir::Expr> generate_arg_list(ASTNode *node);
    sir::Expr generate_star_expr(ASTNode *node);
    sir::Expr generate_primitive_type(ASTNode *node, sir::Primitive primitive);

    char decode_char(const std::string &value, unsigned &index);
};

} // namespace lang

} // namespace banjo

#endif
