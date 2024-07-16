#ifndef SIR_PRINTER_H
#define SIR_PRINTER_H

#include "banjo/sir/sir.hpp"

#include <ostream>

namespace banjo {

namespace lang {

namespace sir {

class Printer {

private:
    std::ostream &stream;
    unsigned indent = 0;

public:
    Printer(std::ostream &stream);
    void print(const Unit &unit);

private:
    void print_decl_block(const DeclBlock &decl_block);
    void print_decl(const Decl &decl);
    void print_func_def(const FuncDef &func_def);
    void print_native_func_decl(const NativeFuncDecl &native_func_decl);
    void print_struct_def(const StructDef &struct_def);
    void print_struct_field(const StructField &struct_field);
    void print_var_decl(const VarDecl &var_decl);

    void print_block(const Block &block);
    void print_stmt(const Stmt &stmt);
    void print_var_stmt(const VarStmt &var_stmt);
    void print_assign_stmt(const AssignStmt &assign_stmt);
    void print_return_stmt(const ReturnStmt &return_stmt);
    void print_if_stmt(const IfStmt &if_stmt);
    void print_expr_stmt(const Expr &expr);

    void print_expr(const Expr &expr);
    void print_int_literal(const IntLiteral &int_literal);
    void print_string_literal(const StringLiteral &string_literal);
    void print_struct_literal(const StructLiteral &struct_literal);
    void print_ident_expr(const IdentExpr &ident_expr);
    void print_binary_expr(const BinaryExpr &binary_expr);
    void print_unary_expr(const UnaryExpr &unary_expr);
    void print_call_expr(const CallExpr &call_expr);
    void print_dot_expr(const DotExpr &dot_expr);
    void print_primitive_type(const PrimitiveType &primitive_type);
    void print_pointer_type(const PointerType &pointer_type);
    void print_func_type(const FuncType &func_type);
    void print_star_expr(const StarExpr &star_expr);

    std::string get_indent();
    void new_line();
};

} // namespace sir

} // namespace lang

} // namespace banjo

#endif
