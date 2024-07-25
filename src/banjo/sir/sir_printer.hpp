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
    void print_mod(const Module &mod);

    void print_decl_block(const DeclBlock &decl_block);
    void print_decl(const Decl &decl);
    void print_func_def(const FuncDef &func_def);
    void print_native_func_decl(const NativeFuncDecl &native_func_decl);
    void print_struct_def(const StructDef &struct_def);
    void print_struct_field(const StructField &struct_field);
    void print_var_decl(const VarDecl &var_decl);
    void print_enum_def(const EnumDef &enum_def);
    void print_enum_variant(const EnumVariant &enum_variant);
    void print_use_decl(const UseDecl &use_decl);

    void print_use_item(const UseItem &use_item);
    void print_use_ident(const UseIdent &use_ident);
    void print_use_rebind(const UseRebind &use_rebind);
    void print_use_dot_expr(const UseDotExpr &use_dot_expr);
    void print_use_list(const UseList &use_list);

    void print_block(const Block &block);
    void print_stmt(const Stmt &stmt);
    void print_var_stmt(const VarStmt &var_stmt);
    void print_assign_stmt(const AssignStmt &assign_stmt);
    void print_comp_assign_stmt(const CompAssignStmt &comp_assign_stmt);
    void print_return_stmt(const ReturnStmt &return_stmt);
    void print_if_stmt(const IfStmt &if_stmt);
    void print_while_stmt(const WhileStmt &while_stmt);
    void print_for_stmt(const ForStmt &for_stmt);
    void print_loop_stmt(const LoopStmt &loop_stmt);
    void print_continue_stmt(const ContinueStmt &continue_stmt);
    void print_break_stmt(const BreakStmt &break_stmt);
    void print_expr_stmt(const Expr &expr);
    void print_block_stmt(const Block &block);

    void print_expr(const Expr &expr);
    void print_int_literal(const IntLiteral &int_literal);
    void print_fp_literal(const FPLiteral &fp_literal);
    void print_bool_literal(const BoolLiteral &bool_literal);
    void print_char_literal(const CharLiteral &char_literal);
    void print_string_literal(const StringLiteral &string_literal);
    void print_struct_literal(const StructLiteral &struct_literal);
    void print_symbol_expr(const SymbolExpr &symbol_expr);
    void print_binary_expr(const BinaryExpr &binary_expr);
    void print_unary_expr(const UnaryExpr &unary_expr);
    void print_cast_expr(const CastExpr &cast_expr);
    void print_index_expr(const IndexExpr &index_expr);
    void print_call_expr(const CallExpr &call_expr);
    void print_field_expr(const FieldExpr &field_expr);
    void print_range_expr(const RangeExpr &range_expr);
    void print_primitive_type(const PrimitiveType &primitive_type);
    void print_pointer_type(const PointerType &pointer_type);
    void print_func_type(const FuncType &func_type);
    void print_ident_expr(const IdentExpr &ident_expr);
    void print_star_expr(const StarExpr &star_expr);
    void print_bracket_expr(const BracketExpr &bracket_expr);
    void print_dot_expr(const DotExpr &dot_expr);

    void print_binary_op(const char *field_name, BinaryOp op);

    std::string get_indent();
    void new_line();
};

} // namespace sir

} // namespace lang

} // namespace banjo

#endif
