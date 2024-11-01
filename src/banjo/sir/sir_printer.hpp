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
    void print_func_decl(const FuncDecl &func_decl);
    void print_native_func_decl(const NativeFuncDecl &native_func_decl);
    void print_const_def(const ConstDef &const_def);
    void print_struct_def(const StructDef &struct_def);
    void print_struct_field(const StructField &struct_field);
    void print_var_decl(const VarDecl &var_decl);
    void print_native_var_decl(const NativeVarDecl &native_var_decl);
    void print_enum_def(const EnumDef &enum_def);
    void print_enum_variant(const EnumVariant &enum_variant);
    void print_union_def(const UnionDef &union_def);
    void print_union_case(const UnionCase &union_case);
    void print_proto_def(const ProtoDef &proto_def);
    void print_type_alias(const TypeAlias &type_alias);
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
    void print_switch_stmt(const SwitchStmt &switch_stmt);
    void print_try_stmt(const TryStmt &try_stmt);
    void print_while_stmt(const WhileStmt &while_stmt);
    void print_for_stmt(const ForStmt &for_stmt);
    void print_loop_stmt(const LoopStmt &loop_stmt);
    void print_continue_stmt(const ContinueStmt &continue_stmt);
    void print_break_stmt(const BreakStmt &break_stmt);
    void print_meta_if_stmt(const MetaIfStmt &meta_if_stmt);
    void print_meta_for_stmt(const MetaForStmt &meta_for_stmt);
    void print_expanded_meta_stmt(const ExpandedMetaStmt &expanded_meta_stmt);
    void print_expr_stmt(const Expr &expr);
    void print_block_stmt(const Block &block);

    void print_expr(const Expr &expr);
    void print_int_literal(const IntLiteral &int_literal);
    void print_fp_literal(const FPLiteral &fp_literal);
    void print_bool_literal(const BoolLiteral &bool_literal);
    void print_char_literal(const CharLiteral &char_literal);
    void print_null_literal(const NullLiteral &null_literal);
    void print_none_literal(const NoneLiteral &none_literal);
    void print_undefined_literal(const UndefinedLiteral &undefined_literal);
    void print_array_literal(const ArrayLiteral &array_literal);
    void print_string_literal(const StringLiteral &string_literal);
    void print_struct_literal(const StructLiteral &struct_literal);
    void print_union_case_literal(const UnionCaseLiteral &union_case_literal);
    void print_map_literal(const MapLiteral &map_literal);
    void print_closure_literal(const ClosureLiteral &closure_literal);
    void print_symbol_expr(const SymbolExpr &symbol_expr);
    void print_binary_expr(const BinaryExpr &binary_expr);
    void print_unary_expr(const UnaryExpr &unary_expr);
    void print_cast_expr(const CastExpr &cast_expr);
    void print_index_expr(const IndexExpr &index_expr);
    void print_call_expr(const CallExpr &call_expr);
    void print_field_expr(const FieldExpr &field_expr);
    void print_range_expr(const RangeExpr &range_expr);
    void print_tuple_expr(const TupleExpr &tuple_expr);
    void print_coercion_expr(const CoercionExpr &coercion_expr);
    void print_primitive_type(const PrimitiveType &primitive_type);
    void print_pointer_type(const PointerType &pointer_type);
    void print_static_array_type(const StaticArrayType &static_array_type);
    void print_func_type(const FuncType &func_type);
    void print_optional_type(const OptionalType &optional_type);
    void print_result_type(const ResultType &result_type);
    void print_array_type(const ArrayType &array_type);
    void print_map_type(const MapType &map_type);
    void print_closure_type(const ClosureType &closure_type);
    void print_ident_expr(const IdentExpr &ident_expr);
    void print_star_expr(const StarExpr &star_expr);
    void print_bracket_expr(const BracketExpr &bracket_expr);
    void print_dot_expr(const DotExpr &dot_expr);
    void print_pseudo_type(const PseudoType &pseudo_type);
    void print_meta_access(const MetaAccess &meta_access);
    void print_meta_field_expr(const MetaFieldExpr &meta_field_expr);
    void print_meta_call_expr(const MetaCallExpr &meta_call_expr);
    void print_move_expr(const MoveExpr &move_expr);
    void print_deinit_expr(const DeinitExpr &deinit_expr);

    void print_generic_params(const std::vector<GenericParam> &generic_params);
    void print_attrs(const Attributes &attrs);
    void print_local(const Local &local);
    void print_binary_op(const char *field_name, BinaryOp op);
    void print_meta_block(const MetaBlock &meta_block);
    void print_error(const Error &error);

    std::string get_indent();
    void new_line();
};

} // namespace sir

} // namespace lang

} // namespace banjo

#endif
