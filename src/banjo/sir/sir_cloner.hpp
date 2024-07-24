#ifndef SIR_CLONER_H
#define SIR_CLONER_H

#include "banjo/sir/sir.hpp"

#include <stack>

namespace banjo {

namespace lang {

namespace sir {

class SIRCloner {

private:
    Module &mod;
    std::stack<SymbolTable*> symbol_tables;

public:
    SIRCloner(Module &mod);

    Block clone_block(const Block &block);
    Stmt clone_stmt(const Stmt &stmt);
    VarStmt *clone_var_stmt(const VarStmt &var_stmt);
    AssignStmt *clone_assign_stmt(const AssignStmt &assign_stmt);
    CompAssignStmt *clone_comp_assign_stmt(const CompAssignStmt &comp_assign_stmt);
    ReturnStmt *clone_return_stmt(const ReturnStmt &return_stmt);
    IfStmt *clone_if_stmt(const IfStmt &if_stmt);
    WhileStmt *clone_while_stmt(const WhileStmt &while_stmt);
    ForStmt *clone_for_stmt(const ForStmt &for_stmt);
    LoopStmt *clone_loop_stmt(const LoopStmt &loop_stmt);
    ContinueStmt *clone_continue_stmt(const ContinueStmt &continue_stmt);
    BreakStmt *clone_break_stmt(const BreakStmt &break_stmt);
    Expr *clone_expr_stmt(const Expr &expr);
    Block *clone_block_stmt(const Block &block);

    Expr clone_expr(const Expr &expr);
    IntLiteral *clone_int_literal(const IntLiteral &int_literal);
    FPLiteral *clone_fp_literal(const FPLiteral &fp_literal);
    BoolLiteral *clone_bool_literal(const BoolLiteral &bool_literal);
    CharLiteral *clone_char_literal(const CharLiteral &char_literal);
    StringLiteral *clone_string_literal(const StringLiteral &string_literal);
    StructLiteral *clone_struct_literal(const StructLiteral &struct_literal);
    SymbolExpr *clone_symbol_expr(const SymbolExpr &symbol_expr);
    BinaryExpr *clone_binary_expr(const BinaryExpr &binary_expr);
    UnaryExpr *clone_unary_expr(const UnaryExpr &unary_expr);
    CastExpr *clone_cast_expr(const CastExpr &cast_expr);
    IndexExpr *clone_index_expr(const IndexExpr &index_expr);
    CallExpr *clone_call_expr(const CallExpr &call_expr);
    FieldExpr *clone_field_expr(const FieldExpr &field_expr);
    RangeExpr *clone_range_expr(const RangeExpr &range_expr);
    PrimitiveType *clone_primitive_type(const PrimitiveType &primitive_type);
    PointerType *clone_pointer_type(const PointerType &pointer_type);
    FuncType *clone_func_type(const FuncType &func_type);
    IdentExpr *clone_ident_expr(const IdentExpr &ident_expr);
    StarExpr *clone_star_expr(const StarExpr &star_expr);
    BracketExpr *clone_bracket_expr(const BracketExpr &bracket_expr);
    DotExpr *clone_dot_expr(const DotExpr &dot_expr);
};

} // namespace sir

} // namespace lang

} // namespace banjo

#endif
