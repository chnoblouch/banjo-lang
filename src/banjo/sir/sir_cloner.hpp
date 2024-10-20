#ifndef SIR_CLONER_H
#define SIR_CLONER_H

#include "banjo/sir/sir.hpp"
#include "sir.hpp"

#include <stack>

namespace banjo {

namespace lang {

namespace sir {

class Cloner {

private:
    Module &mod;
    std::stack<SymbolTable *> symbol_tables;

public:
    Cloner(Module &mod);

    DeclBlock clone_decl_block(const DeclBlock &decl_block);
    Decl clone_decl(const Decl &decl);
    FuncDef *clone_func_def(const FuncDef &func_def);
    NativeFuncDecl *clone_native_func_decl(const NativeFuncDecl &native_func_decl);
    ConstDef *clone_const_def(const ConstDef &const_def);
    StructDef *clone_struct_def(const StructDef &struct_def);
    StructField *clone_struct_field(const StructField &struct_field);
    VarDecl *clone_var_decl(const VarDecl &var_decl);
    NativeVarDecl *clone_native_var_decl(const NativeVarDecl &native_var_decl);
    EnumDef *clone_enum_def(const EnumDef &enum_def);
    EnumVariant *clone_enum_variant(const EnumVariant &enum_variant);
    UnionDef *clone_union_def(const UnionDef &union_def);
    UnionCase *clone_union_case(const UnionCase &union_case);
    TypeAlias *clone_type_alias(const TypeAlias &type_alias);
    MetaIfStmt *clone_meta_if_stmt(const MetaIfStmt &meta_if_stmt);
    MetaForStmt *clone_meta_for_stmt(const MetaForStmt &meta_for_stmt);

    Block clone_block(const Block &block);
    Stmt clone_stmt(const Stmt &stmt);
    VarStmt *clone_var_stmt(const VarStmt &var_stmt);
    AssignStmt *clone_assign_stmt(const AssignStmt &assign_stmt);
    CompAssignStmt *clone_comp_assign_stmt(const CompAssignStmt &comp_assign_stmt);
    ReturnStmt *clone_return_stmt(const ReturnStmt &return_stmt);
    IfStmt *clone_if_stmt(const IfStmt &if_stmt);
    SwitchStmt *clone_switch_stmt(const SwitchStmt &switch_stmt);
    TryStmt *clone_try_stmt(const TryStmt &try_stmt);
    WhileStmt *clone_while_stmt(const WhileStmt &while_stmt);
    ForStmt *clone_for_stmt(const ForStmt &for_stmt);
    LoopStmt *clone_loop_stmt(const LoopStmt &loop_stmt);
    ContinueStmt *clone_continue_stmt(const ContinueStmt &continue_stmt);
    BreakStmt *clone_break_stmt(const BreakStmt &break_stmt);
    Expr *clone_expr_stmt(const Expr &expr);
    Block *clone_block_stmt(const Block &block);

    Expr clone_expr(const Expr &expr);
    std::vector<Expr> clone_expr_list(const std::vector<Expr> &exprs);
    IntLiteral *clone_int_literal(const IntLiteral &int_literal);
    FPLiteral *clone_fp_literal(const FPLiteral &fp_literal);
    BoolLiteral *clone_bool_literal(const BoolLiteral &bool_literal);
    CharLiteral *clone_char_literal(const CharLiteral &char_literal);
    NullLiteral *clone_null_literal(const NullLiteral &null_literal);
    NoneLiteral *clone_none_literal(const NoneLiteral &none_literal);
    UndefinedLiteral *clone_undefined_literal(const UndefinedLiteral &undefined_literal);
    ArrayLiteral *clone_array_literal(const ArrayLiteral &array_literal);
    StringLiteral *clone_string_literal(const StringLiteral &string_literal);
    StructLiteral *clone_struct_literal(const StructLiteral &struct_literal);
    UnionCaseLiteral *clone_union_case_literal(const UnionCaseLiteral &union_case_literal);
    MapLiteral *clone_map_literal(const MapLiteral &map_literal);
    ClosureLiteral *clone_closure_literal(const ClosureLiteral &closure_literal);
    SymbolExpr *clone_symbol_expr(const SymbolExpr &symbol_expr);
    BinaryExpr *clone_binary_expr(const BinaryExpr &binary_expr);
    UnaryExpr *clone_unary_expr(const UnaryExpr &unary_expr);
    CastExpr *clone_cast_expr(const CastExpr &cast_expr);
    IndexExpr *clone_index_expr(const IndexExpr &index_expr);
    CallExpr *clone_call_expr(const CallExpr &call_expr);
    FieldExpr *clone_field_expr(const FieldExpr &field_expr);
    RangeExpr *clone_range_expr(const RangeExpr &range_expr);
    TupleExpr *clone_tuple_expr(const TupleExpr &tuple_expr);
    CoercionExpr *clone_coercion_expr(const CoercionExpr &coercion_expr);
    PrimitiveType *clone_primitive_type(const PrimitiveType &primitive_type);
    PointerType *clone_pointer_type(const PointerType &pointer_type);
    StaticArrayType *clone_static_array_type(const StaticArrayType &static_array_type);
    FuncType *clone_func_type(const FuncType &func_type);
    OptionalType *clone_optional_type(const OptionalType &optional_type);
    ResultType *clone_result_type(const ResultType &result_type);
    ArrayType *clone_array_type(const ArrayType &array_type);
    MapType *clone_map_type(const MapType &map_type);
    ClosureType *clone_closure_type(const ClosureType &closure_type);
    IdentExpr *clone_ident_expr(const IdentExpr &ident_expr);
    StarExpr *clone_star_expr(const StarExpr &star_expr);
    BracketExpr *clone_bracket_expr(const BracketExpr &bracket_expr);
    DotExpr *clone_dot_expr(const DotExpr &dot_expr);
    PseudoType *clone_pseudo_type(const PseudoType &pseudo_type);
    MetaAccess *clone_meta_access(const MetaAccess &meta_access);
    MetaFieldExpr *clone_meta_field_expr(const MetaFieldExpr &meta_field_expr);
    MetaCallExpr *clone_meta_call_expr(const MetaCallExpr &meta_call_expr);
    MoveExpr *clone_move_expr(const MoveExpr &move_expr);
    DeinitExpr *clone_deinit_expr(const DeinitExpr &deinit_expr);

    SymbolTable *push_symbol_table(SymbolTable *parent_if_empty);
    void pop_symbol_table() { symbol_tables.pop(); }

    Local clone_local(const Local &local);
    Attributes *clone_attrs(const Attributes *attrs);
    MetaBlock clone_meta_block(const MetaBlock &meta_block);
    Error *clone_error(const Error &error);
};

} // namespace sir

} // namespace lang

} // namespace banjo

#endif
