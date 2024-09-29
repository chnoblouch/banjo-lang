#include "sir_cloner.hpp"

#include "banjo/sir/sir_visitor.hpp"
#include "banjo/utils/macros.hpp"
#include "sir.hpp"

#include <cassert>
#include <vector>

namespace banjo {

namespace lang {

namespace sir {

Cloner::Cloner(Module &mod) : mod(mod) {}

DeclBlock Cloner::clone_decl_block(const DeclBlock &decl_block) {
    assert(decl_block.symbol_table->symbols.empty());

    SymbolTable *symbol_table = push_symbol_table(decl_block.symbol_table->parent);

    std::vector<Decl> decls;
    decls.resize(decl_block.decls.size());

    for (unsigned i = 0; i < decl_block.decls.size(); i++) {
        decls[i] = clone_decl(decl_block.decls[i]);
    }

    pop_symbol_table();

    return DeclBlock{
        .ast_node = decl_block.ast_node,
        .decls = decls,
        .symbol_table = symbol_table,
    };
}

Decl Cloner::clone_decl(const Decl &decl) {
    SIR_VISIT_DECL(
        decl,
        return nullptr,
        return clone_func_def(*inner),
        return clone_native_func_decl(*inner),
        return clone_const_def(*inner),
        return clone_struct_def(*inner),
        return clone_struct_field(*inner),
        return clone_var_decl(*inner),
        return clone_native_var_decl(*inner),
        return clone_enum_def(*inner),
        return clone_enum_variant(*inner),
        return clone_union_def(*inner),
        return clone_union_case(*inner),
        return clone_type_alias(*inner),
        SIR_VISIT_IMPOSSIBLE,
        return clone_meta_if_stmt(*inner),
        SIR_VISIT_IMPOSSIBLE
    );
}

FuncDef *Cloner::clone_func_def(const FuncDef &func_def) {
    assert(func_def.specializations.empty());

    return mod.create_decl(FuncDef{
        .ast_node = func_def.ast_node,
        .ident = func_def.ident,
        .type = *clone_func_type(func_def.type), // FIXME: unneccessary heap allocation
        .block = clone_block(func_def.block),
        .attrs = clone_attrs(func_def.attrs),
        .generic_params = func_def.generic_params,
        .specializations = {},
    });
}

NativeFuncDecl *Cloner::clone_native_func_decl(const NativeFuncDecl &native_func_decl) {
    return mod.create_decl(NativeFuncDecl{
        .ast_node = native_func_decl.ast_node,
        .ident = native_func_decl.ident,
        .type = *clone_func_type(native_func_decl.type), // FIXME: unneccessary heap allocation
        .attrs = clone_attrs(native_func_decl.attrs),
    });
}

ConstDef *Cloner::clone_const_def(const ConstDef &const_def) {
    return mod.create_decl(ConstDef{
        .ast_node = const_def.ast_node,
        .ident = const_def.ident,
        .type = clone_expr(const_def.type),
        .value = clone_expr(const_def.value),
    });
}

StructDef *Cloner::clone_struct_def(const StructDef &struct_def) {
    assert(struct_def.fields.empty());
    assert(struct_def.specializations.empty());

    return mod.create_decl(StructDef{
        .ast_node = struct_def.ast_node,
        .ident = struct_def.ident,
        .block = clone_decl_block(struct_def.block),
        .fields = {},
        .generic_params = struct_def.generic_params,
        .specializations = {},
    });
}

StructField *Cloner::clone_struct_field(const StructField & /*struct_field*/) {
    ASSERT_UNREACHABLE;
}

VarDecl *Cloner::clone_var_decl(const VarDecl &var_decl) {
    return mod.create_decl(VarDecl{
        .ast_node = var_decl.ast_node,
        .ident = var_decl.ident,
        .type = clone_expr(var_decl.type),
        .value = clone_expr(var_decl.value),
    });
}

NativeVarDecl *Cloner::clone_native_var_decl(const NativeVarDecl &native_var_decl) {
    return mod.create_decl(NativeVarDecl{
        .ast_node = native_var_decl.ast_node,
        .ident = native_var_decl.ident,
        .type = clone_expr(native_var_decl.type),
        .attrs = clone_attrs(native_var_decl.attrs),
    });
}

EnumDef *Cloner::clone_enum_def(const EnumDef &enum_def) {
    assert(enum_def.variants.empty());

    return mod.create_decl(EnumDef{
        .ast_node = enum_def.ast_node,
        .ident = enum_def.ident,
        .block = enum_def.block,
        .variants = {},
    });
}

EnumVariant *Cloner::clone_enum_variant(const EnumVariant &enum_variant) {
    return mod.create_decl(EnumVariant{
        .ast_node = enum_variant.ast_node,
        .ident = enum_variant.ident,
        .type = clone_expr(enum_variant.type),
        .value = clone_expr(enum_variant.value),
    });
}

UnionDef *Cloner::clone_union_def(const UnionDef &union_def) {
    return mod.create_decl(UnionDef{
        .ast_node = union_def.ast_node,
        .ident = union_def.ident,
        .block = clone_decl_block(union_def.block),
        .cases = {},
    });
}

UnionCase *Cloner::clone_union_case(const UnionCase &union_case) {
    return mod.create_decl(UnionCase{
        .ast_node = union_case.ast_node,
        .ident = union_case.ident,
        .fields = union_case.fields,
    });
}

TypeAlias *Cloner::clone_type_alias(const TypeAlias &type_alias) {
    return mod.create_decl(TypeAlias{
        .ast_node = type_alias.ast_node,
        .ident = type_alias.ident,
        .type = clone_expr(type_alias.type),
    });
}

MetaIfStmt *Cloner::clone_meta_if_stmt(const MetaIfStmt &meta_if_stmt) {
    std::vector<MetaIfCondBranch> cond_branches;
    cond_branches.resize(meta_if_stmt.cond_branches.size());

    for (unsigned i = 0; i < meta_if_stmt.cond_branches.size(); i++) {
        const MetaIfCondBranch &if_cond_branch = meta_if_stmt.cond_branches[i];

        cond_branches[i] = MetaIfCondBranch{
            .ast_node = if_cond_branch.ast_node,
            .condition = clone_expr(if_cond_branch.condition),
            .block = clone_meta_block(if_cond_branch.block),
        };
    }

    std::optional<MetaIfElseBranch> else_branch;

    if (meta_if_stmt.else_branch) {
        else_branch = MetaIfElseBranch{
            .ast_node = meta_if_stmt.else_branch->ast_node,
            .block = clone_meta_block(meta_if_stmt.else_branch->block),
        };
    }

    return mod.create_stmt(MetaIfStmt{
        .ast_node = meta_if_stmt.ast_node,
        .cond_branches = cond_branches,
        .else_branch = else_branch,
    });
}

MetaForStmt *Cloner::clone_meta_for_stmt(const MetaForStmt &meta_for_stmt) {
    return mod.create_stmt(MetaForStmt{
        .ast_node = meta_for_stmt.ast_node,
        .ident = meta_for_stmt.ident,
        .range = clone_expr(meta_for_stmt.range),
        .block = clone_meta_block(meta_for_stmt.block),
    });
}

Block Cloner::clone_block(const Block &block) {
    assert(block.symbol_table->symbols.empty());

    SymbolTable *symbol_table = push_symbol_table(block.symbol_table->parent);

    std::vector<Stmt> stmts;
    stmts.resize(block.stmts.size());

    for (unsigned i = 0; i < block.stmts.size(); i++) {
        stmts[i] = clone_stmt(block.stmts[i]);
    }

    pop_symbol_table();

    return Block{
        .ast_node = block.ast_node,
        .stmts = stmts,
        .symbol_table = symbol_table,
    };
}

Stmt Cloner::clone_stmt(const Stmt &stmt) {
    SIR_VISIT_STMT(
        stmt,
        SIR_VISIT_IMPOSSIBLE,
        return clone_var_stmt(*inner),
        return clone_assign_stmt(*inner),
        return clone_comp_assign_stmt(*inner),
        return clone_return_stmt(*inner),
        return clone_if_stmt(*inner),
        return clone_switch_stmt(*inner),
        return clone_try_stmt(*inner),
        return clone_while_stmt(*inner),
        return clone_for_stmt(*inner),
        return clone_loop_stmt(*inner),
        return clone_continue_stmt(*inner),
        return clone_break_stmt(*inner),
        return clone_meta_if_stmt(*inner),
        return clone_meta_for_stmt(*inner),
        SIR_VISIT_IMPOSSIBLE,
        return clone_expr_stmt(*inner),
        return clone_block_stmt(*inner)
    );
}

VarStmt *Cloner::clone_var_stmt(const VarStmt &var_stmt) {
    return mod.create_stmt(VarStmt{
        .ast_node = var_stmt.ast_node,
        .local = clone_local(var_stmt.local),
        .value = clone_expr(var_stmt.value),
    });
}

AssignStmt *Cloner::clone_assign_stmt(const AssignStmt &assign_stmt) {
    return mod.create_stmt(AssignStmt{
        .ast_node = assign_stmt.ast_node,
        .lhs = clone_expr(assign_stmt.lhs),
        .rhs = clone_expr(assign_stmt.rhs),
    });
}

CompAssignStmt *Cloner::clone_comp_assign_stmt(const CompAssignStmt &comp_assign_stmt) {
    return mod.create_stmt(CompAssignStmt{
        .ast_node = comp_assign_stmt.ast_node,
        .op = comp_assign_stmt.op,
        .lhs = clone_expr(comp_assign_stmt.lhs),
        .rhs = clone_expr(comp_assign_stmt.rhs),
    });
}

ReturnStmt *Cloner::clone_return_stmt(const ReturnStmt &return_stmt) {
    return mod.create_stmt(ReturnStmt{
        .ast_node = return_stmt.ast_node,
        .value = clone_expr(return_stmt.value),
    });
}

IfStmt *Cloner::clone_if_stmt(const IfStmt &if_stmt) {
    std::vector<IfCondBranch> cond_branches(if_stmt.cond_branches.size());

    for (unsigned i = 0; i < if_stmt.cond_branches.size(); i++) {
        const IfCondBranch &if_cond_branch = if_stmt.cond_branches[i];

        cond_branches[i] = IfCondBranch{
            .ast_node = if_cond_branch.ast_node,
            .condition = clone_expr(if_cond_branch.condition),
            .block = clone_block(if_cond_branch.block),
        };
    }

    std::optional<IfElseBranch> else_branch;

    if (if_stmt.else_branch) {
        else_branch = IfElseBranch{
            .ast_node = if_stmt.else_branch->ast_node,
            .block = clone_block(if_stmt.else_branch->block),
        };
    }

    return mod.create_stmt(IfStmt{
        .ast_node = if_stmt.ast_node,
        .cond_branches = cond_branches,
        .else_branch = else_branch,
    });
}

SwitchStmt *Cloner::clone_switch_stmt(const SwitchStmt &switch_stmt) {
    std::vector<SwitchCaseBranch> case_branches(switch_stmt.case_branches.size());

    for (unsigned i = 0; i < switch_stmt.case_branches.size(); i++) {
        const SwitchCaseBranch &switch_cond_branch = switch_stmt.case_branches[i];

        case_branches[i] = SwitchCaseBranch{
            .ast_node = switch_cond_branch.ast_node,
            .local = clone_local(switch_cond_branch.local),
            .block = clone_block(switch_cond_branch.block),
        };
    }

    return mod.create_stmt(SwitchStmt{
        .ast_node = switch_stmt.ast_node,
        .value = clone_expr(switch_stmt.value),
        .case_branches = case_branches,
    });
}

TryStmt *Cloner::clone_try_stmt(const TryStmt &try_stmt) {
    TrySuccessBranch success_branch{
        .ast_node = try_stmt.success_branch.ast_node,
        .ident = try_stmt.success_branch.ident,
        .expr = clone_expr(try_stmt.success_branch.expr),
        .block = clone_block(try_stmt.success_branch.block),
    };

    std::optional<TryExceptBranch> except_branch;

    if (try_stmt.except_branch) {
        except_branch = TryExceptBranch{
            .ast_node = try_stmt.except_branch->ast_node,
            .ident = try_stmt.except_branch->ident,
            .type = clone_expr(try_stmt.except_branch->type),
            .block = clone_block(try_stmt.except_branch->block),
        };
    }

    std::optional<TryElseBranch> else_branch;

    if (try_stmt.else_branch) {
        else_branch = TryElseBranch{
            .ast_node = try_stmt.else_branch->ast_node,
            .block = clone_block(try_stmt.else_branch->block),
        };
    }

    return mod.create_stmt(TryStmt{
        .ast_node = try_stmt.ast_node,
        .success_branch = success_branch,
        .except_branch = except_branch,
        .else_branch = else_branch,
    });
}

WhileStmt *Cloner::clone_while_stmt(const WhileStmt &while_stmt) {
    return mod.create_stmt(WhileStmt{
        .ast_node = while_stmt.ast_node,
        .condition = clone_expr(while_stmt.condition),
        .block = clone_block(while_stmt.block),
    });
}

ForStmt *Cloner::clone_for_stmt(const ForStmt &for_stmt) {
    return mod.create_stmt(ForStmt{
        .ast_node = for_stmt.ast_node,
        .ident = for_stmt.ident,
        .range = clone_expr(for_stmt.range),
        .block = clone_block(for_stmt.block),
    });
}

LoopStmt *Cloner::clone_loop_stmt(const LoopStmt & /*loop_stmt*/) {
    ASSERT_UNREACHABLE;
}

ContinueStmt *Cloner::clone_continue_stmt(const ContinueStmt &continue_stmt) {
    return mod.create_stmt(ContinueStmt{
        .ast_node = continue_stmt.ast_node,
    });
}

BreakStmt *Cloner::clone_break_stmt(const BreakStmt &break_stmt) {
    return mod.create_stmt(BreakStmt{
        .ast_node = break_stmt.ast_node,
    });
}

Expr *Cloner::clone_expr_stmt(const Expr &expr) {
    return mod.create_stmt(clone_expr(expr));
}

Block *Cloner::clone_block_stmt(const Block &block) {
    return mod.create_stmt(clone_block(block));
}

Expr Cloner::clone_expr(const Expr &expr) {
    SIR_VISIT_EXPR(
        expr,
        return nullptr,
        return clone_int_literal(*inner),
        return clone_fp_literal(*inner),
        return clone_bool_literal(*inner),
        return clone_char_literal(*inner),
        return clone_null_literal(*inner),
        return clone_none_literal(*inner),
        return clone_undefined_literal(*inner),
        return clone_array_literal(*inner),
        return clone_string_literal(*inner),
        return clone_struct_literal(*inner),
        return clone_union_case_literal(*inner),
        return clone_map_literal(*inner),
        return clone_closure_literal(*inner),
        return clone_symbol_expr(*inner),
        return clone_binary_expr(*inner),
        return clone_unary_expr(*inner),
        return clone_cast_expr(*inner),
        return clone_index_expr(*inner),
        return clone_call_expr(*inner),
        return clone_field_expr(*inner),
        return clone_range_expr(*inner),
        return clone_tuple_expr(*inner),
        return clone_coercion_expr(*inner),
        return clone_primitive_type(*inner),
        return clone_pointer_type(*inner),
        return clone_static_array_type(*inner),
        return clone_func_type(*inner),
        return clone_optional_type(*inner),
        return clone_result_type(*inner),
        return clone_array_type(*inner),
        return clone_map_type(*inner),
        return clone_closure_type(*inner),
        return clone_ident_expr(*inner),
        return clone_star_expr(*inner),
        return clone_bracket_expr(*inner),
        return clone_dot_expr(*inner),
        return clone_meta_access(*inner),
        return clone_meta_field_expr(*inner),
        return clone_meta_call_expr(*inner)
    );
}

IntLiteral *Cloner::clone_int_literal(const IntLiteral &int_literal) {
    return mod.create_expr(IntLiteral{
        .ast_node = int_literal.ast_node,
        .type = clone_expr(int_literal.type),
        .value = int_literal.value,
    });
}

FPLiteral *Cloner::clone_fp_literal(const FPLiteral &fp_literal) {
    return mod.create_expr(FPLiteral{
        .ast_node = fp_literal.ast_node,
        .type = clone_expr(fp_literal.type),
        .value = fp_literal.value,
    });
}

BoolLiteral *Cloner::clone_bool_literal(const BoolLiteral &bool_literal) {
    return mod.create_expr(BoolLiteral{
        .ast_node = bool_literal.ast_node,
        .type = clone_expr(bool_literal.type),
        .value = bool_literal.value,
    });
}

CharLiteral *Cloner::clone_char_literal(const CharLiteral &char_literal) {
    return mod.create_expr(CharLiteral{
        .ast_node = char_literal.ast_node,
        .type = clone_expr(char_literal.type),
        .value = char_literal.value,
    });
}

NullLiteral *Cloner::clone_null_literal(const NullLiteral &null_literal) {
    return mod.create_expr(NullLiteral{
        .ast_node = null_literal.ast_node,
        .type = clone_expr(null_literal.type),
    });
}

NoneLiteral *Cloner::clone_none_literal(const NoneLiteral &none_literal) {
    return mod.create_expr(NoneLiteral{
        .ast_node = none_literal.ast_node,
        .type = clone_expr(none_literal.type),
    });
}

UndefinedLiteral *Cloner::clone_undefined_literal(const UndefinedLiteral &undefined_literal) {
    return mod.create_expr(UndefinedLiteral{
        .ast_node = undefined_literal.ast_node,
        .type = clone_expr(undefined_literal.type),
    });
}

ArrayLiteral *Cloner::clone_array_literal(const ArrayLiteral &array_literal) {
    return mod.create_expr(ArrayLiteral{
        .ast_node = array_literal.ast_node,
        .type = clone_expr(array_literal.type),
        .values = clone_expr_list(array_literal.values),
    });
}

StringLiteral *Cloner::clone_string_literal(const StringLiteral &string_literal) {
    return mod.create_expr(StringLiteral{
        .ast_node = string_literal.ast_node,
        .type = clone_expr(string_literal.type),
        .value = string_literal.value,
    });
}

StructLiteral *Cloner::clone_struct_literal(const StructLiteral &struct_literal) {
    std::vector<StructLiteralEntry> entries(struct_literal.entries.size());

    for (unsigned i = 0; i < struct_literal.entries.size(); i++) {
        const StructLiteralEntry &entry = struct_literal.entries[i];

        entries[i] = StructLiteralEntry{
            .ident = entry.ident,
            .value = clone_expr(entry.value),
            .field = nullptr,
        };
    }

    return mod.create_expr(StructLiteral{
        .ast_node = struct_literal.ast_node,
        .type = clone_expr(struct_literal.type),
        .entries = entries,
    });
}

UnionCaseLiteral *Cloner::clone_union_case_literal(const UnionCaseLiteral &union_case_literal) {
    return mod.create_expr(UnionCaseLiteral{
        .ast_node = union_case_literal.ast_node,
        .type = clone_expr(union_case_literal.type),
        .args = clone_expr_list(union_case_literal.args),
    });
}

MapLiteral *Cloner::clone_map_literal(const MapLiteral &map_literal) {
    std::vector<MapLiteralEntry> entries(map_literal.entries.size());

    for (unsigned i = 0; i < map_literal.entries.size(); i++) {
        const MapLiteralEntry &entry = map_literal.entries[i];

        entries[i] = MapLiteralEntry{
            .key = clone_expr(entry.key),
            .value = clone_expr(entry.value),
        };
    }

    return mod.create_expr(MapLiteral{
        .ast_node = map_literal.ast_node,
        .type = clone_expr(map_literal.type),
        .entries = entries,
    });
}

ClosureLiteral *Cloner::clone_closure_literal(const ClosureLiteral &closure_literal) {
    return mod.create_expr(ClosureLiteral{
        .ast_node = closure_literal.ast_node,
        .type = clone_expr(closure_literal.type),
        .func_type = *clone_func_type(closure_literal.func_type),
        .block = clone_block(closure_literal.block),
    });
}

SymbolExpr *Cloner::clone_symbol_expr(const SymbolExpr & /*symbol_expr*/) {
    ASSERT_UNREACHABLE;
}

BinaryExpr *Cloner::clone_binary_expr(const BinaryExpr &binary_expr) {
    return mod.create_expr(BinaryExpr{
        .ast_node = binary_expr.ast_node,
        .type = clone_expr(binary_expr.type),
        .op = binary_expr.op,
        .lhs = clone_expr(binary_expr.lhs),
        .rhs = clone_expr(binary_expr.rhs),
    });
}

UnaryExpr *Cloner::clone_unary_expr(const UnaryExpr &unary_expr) {
    return mod.create_expr(UnaryExpr{
        .ast_node = unary_expr.ast_node,
        .type = clone_expr(unary_expr.type),
        .op = unary_expr.op,
        .value = clone_expr(unary_expr.value),
    });
}

CastExpr *Cloner::clone_cast_expr(const CastExpr &cast_expr) {
    return mod.create_expr(CastExpr{
        .ast_node = cast_expr.ast_node,
        .type = clone_expr(cast_expr.type),
        .value = clone_expr(cast_expr.value),
    });
}

IndexExpr *Cloner::clone_index_expr(const IndexExpr &index_expr) {
    return mod.create_expr(IndexExpr{
        .ast_node = index_expr.ast_node,
        .type = clone_expr(index_expr.type),
        .base = clone_expr(index_expr.base),
        .index = clone_expr(index_expr.index),
    });
}

CallExpr *Cloner::clone_call_expr(const CallExpr &call_expr) {
    return mod.create_expr(CallExpr{
        .ast_node = call_expr.ast_node,
        .type = clone_expr(call_expr.type),
        .callee = clone_expr(call_expr.callee),
        .args = clone_expr_list(call_expr.args),
    });
}

FieldExpr *Cloner::clone_field_expr(const FieldExpr & /*field_expr*/) {
    ASSERT_UNREACHABLE;
}

RangeExpr *Cloner::clone_range_expr(const RangeExpr &range_expr) {
    return mod.create_expr(RangeExpr{
        .ast_node = range_expr.ast_node,
        .lhs = clone_expr(range_expr.lhs),
        .rhs = clone_expr(range_expr.rhs),
    });
}

TupleExpr *Cloner::clone_tuple_expr(const TupleExpr &tuple_expr) {
    return mod.create_expr(TupleExpr{
        .ast_node = tuple_expr.ast_node,
        .type = clone_expr(tuple_expr.type),
        .exprs = clone_expr_list(tuple_expr.exprs),
    });
}

CoercionExpr *Cloner::clone_coercion_expr(const CoercionExpr &coercion_expr) {
    return mod.create_expr(CoercionExpr{
        .ast_node = coercion_expr.ast_node,
        .type = clone_expr(coercion_expr.type),
        .value = clone_expr(coercion_expr.value),
    });
}

PrimitiveType *Cloner::clone_primitive_type(const PrimitiveType &primitive_type) {
    return mod.create_expr(PrimitiveType{
        .ast_node = primitive_type.ast_node,
        .primitive = primitive_type.primitive,
    });
}

PointerType *Cloner::clone_pointer_type(const PointerType &pointer_type) {
    return mod.create_expr(PointerType{
        .ast_node = pointer_type.ast_node,
        .base_type = clone_expr(pointer_type.base_type),
    });
}

StaticArrayType *Cloner::clone_static_array_type(const StaticArrayType &static_array_type) {
    return mod.create_expr(StaticArrayType{
        .ast_node = static_array_type.ast_node,
        .base_type = clone_expr(static_array_type.base_type),
        .length = clone_expr(static_array_type.length),
    });
}

FuncType *Cloner::clone_func_type(const FuncType &func_type) {
    std::vector<Param> params;
    params.resize(func_type.params.size());

    for (unsigned i = 0; i < func_type.params.size(); i++) {
        const Param &param = func_type.params[i];

        params[i] = Param{
            .ast_node = param.ast_node,
            .name = param.name,
            .type = clone_expr(param.type),
        };
    }

    return mod.create_expr(FuncType{
        .ast_node = func_type.ast_node,
        .params = params,
        .return_type = clone_expr(func_type.return_type),
    });
}

OptionalType *Cloner::clone_optional_type(const OptionalType &optional_type) {
    return mod.create_expr(OptionalType{
        .ast_node = optional_type.ast_node,
        .base_type = clone_expr(optional_type.base_type),
    });
}

ResultType *Cloner::clone_result_type(const ResultType &result_type) {
    return mod.create_expr(ResultType{
        .ast_node = result_type.ast_node,
        .value_type = clone_expr(result_type.value_type),
        .error_type = clone_expr(result_type.error_type),
    });
}

ArrayType *Cloner::clone_array_type(const ArrayType &array_type) {
    return mod.create_expr(ArrayType{
        .ast_node = array_type.ast_node,
        .base_type = clone_expr(array_type.base_type),
    });
}

MapType *Cloner::clone_map_type(const MapType &map_type) {
    return mod.create_expr(MapType{
        .ast_node = map_type.ast_node,
        .key_type = map_type.key_type,
        .value_type = map_type.value_type,
    });
}

ClosureType *Cloner::clone_closure_type(const ClosureType &closure_type) {
    return mod.create_expr(ClosureType{
        .ast_node = closure_type.ast_node,
        .func_type = *clone_func_type(closure_type.func_type),
    });
}

IdentExpr *Cloner::clone_ident_expr(const IdentExpr &ident_expr) {
    return mod.create_expr(IdentExpr{
        .ast_node = ident_expr.ast_node,
        .value = ident_expr.value,
    });
}

StarExpr *Cloner::clone_star_expr(const StarExpr &star_expr) {
    return mod.create_expr(StarExpr{
        .ast_node = star_expr.ast_node,
        .value = clone_expr(star_expr.value),
    });
}

BracketExpr *Cloner::clone_bracket_expr(const BracketExpr &bracket_expr) {
    return mod.create_expr(BracketExpr{
        .ast_node = bracket_expr.ast_node,
        .lhs = clone_expr(bracket_expr.lhs),
        .rhs = clone_expr_list(bracket_expr.rhs),
    });
}

DotExpr *Cloner::clone_dot_expr(const DotExpr &dot_expr) {
    return mod.create_expr(DotExpr{
        .ast_node = dot_expr.ast_node,
        .lhs = clone_expr(dot_expr.lhs),
        .rhs = dot_expr.rhs,
    });
}

MetaAccess *Cloner::clone_meta_access(const MetaAccess &meta_access) {
    return mod.create_expr(MetaAccess{
        .ast_node = meta_access.ast_node,
        .expr = clone_expr(meta_access.expr),
    });
}

MetaFieldExpr *Cloner::clone_meta_field_expr(const MetaFieldExpr &meta_field_expr) {
    return mod.create_expr(MetaFieldExpr{
        .ast_node = meta_field_expr.ast_node,
        .base = clone_expr(meta_field_expr.base),
        .field = meta_field_expr.field,
    });
}

MetaCallExpr *Cloner::clone_meta_call_expr(const MetaCallExpr &meta_call_expr) {
    return mod.create_expr(MetaCallExpr{
        .ast_node = meta_call_expr.ast_node,
        .callee = clone_expr(meta_call_expr.callee),
        .args = clone_expr_list(meta_call_expr.args),
    });
}

SymbolTable *Cloner::push_symbol_table(SymbolTable *parent_if_empty) {
    SymbolTable *symbol_table = mod.create_symbol_table({
        .parent = symbol_tables.empty() ? parent_if_empty : symbol_tables.top(),
        .symbols = {},
    });

    symbol_tables.push(symbol_table);
    return symbol_table;
}

Local Cloner::clone_local(const Local &local) {
    return Local{
        .name = local.name,
        .type = clone_expr(local.type),
    };
}

std::vector<Expr> Cloner::clone_expr_list(const std::vector<Expr> &exprs) {
    std::vector<Expr> clone;
    clone.resize(exprs.size());

    for (unsigned i = 0; i < exprs.size(); i++) {
        clone[i] = clone_expr(exprs[i]);
    }

    return clone;
}

Attributes *Cloner::clone_attrs(const Attributes *attrs) {
    return attrs ? mod.create_attrs(*attrs) : nullptr;
}

MetaBlock Cloner::clone_meta_block(const MetaBlock &meta_block) {
    std::vector<Node> nodes;
    nodes.resize(meta_block.nodes.size());

    for (unsigned i = 0; i < meta_block.nodes.size(); i++) {
        const Node &node = meta_block.nodes[i];

        if (auto expr = node.match<Expr>()) nodes[i] = clone_expr(*expr);
        else if (auto stmt = node.match<Stmt>()) nodes[i] = clone_stmt(*stmt);
        else if (auto decl = node.match<Decl>()) nodes[i] = clone_decl(*decl);
        else ASSERT_UNREACHABLE;
    }

    return MetaBlock{
        .ast_node = meta_block.ast_node,
        .nodes = nodes,
    };
}

} // namespace sir

} // namespace lang

} // namespace banjo
