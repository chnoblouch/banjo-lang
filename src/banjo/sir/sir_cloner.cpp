#include "sir_cloner.hpp"

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
    if (!decl) return nullptr;
    else if (auto func_def = decl.match<FuncDef>()) return clone_func_def(*func_def);
    else if (auto native_func_decl = decl.match<NativeFuncDecl>()) return clone_native_func_decl(*native_func_decl);
    else if (auto const_def = decl.match<ConstDef>()) return clone_const_def(*const_def);
    else if (auto struct_def = decl.match<StructDef>()) return clone_struct_def(*struct_def);
    else if (auto struct_field = decl.match<StructField>()) return clone_struct_field(*struct_field);
    else if (auto var_decl = decl.match<VarDecl>()) return clone_var_decl(*var_decl);
    else if (auto enum_def = decl.match<EnumDef>()) return clone_enum_def(*enum_def);
    else if (auto enum_variant = decl.match<EnumVariant>()) return clone_enum_variant(*enum_variant);
    else if (auto meta_if_stmt = decl.match<MetaIfStmt>()) return clone_meta_if_stmt(*meta_if_stmt);
    else ASSERT_UNREACHABLE;
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
    if (!stmt) return nullptr;
    else if (auto var_stmt = stmt.match<VarStmt>()) return clone_var_stmt(*var_stmt);
    else if (auto assign_stmt = stmt.match<AssignStmt>()) return clone_assign_stmt(*assign_stmt);
    else if (auto comp_assign_stmt = stmt.match<CompAssignStmt>()) return clone_comp_assign_stmt(*comp_assign_stmt);
    else if (auto return_stmt = stmt.match<ReturnStmt>()) return clone_return_stmt(*return_stmt);
    else if (auto if_stmt = stmt.match<IfStmt>()) return clone_if_stmt(*if_stmt);
    else if (auto while_stmt = stmt.match<WhileStmt>()) return clone_while_stmt(*while_stmt);
    else if (auto for_stmt = stmt.match<ForStmt>()) return clone_for_stmt(*for_stmt);
    else if (auto loop_stmt = stmt.match<LoopStmt>()) return clone_loop_stmt(*loop_stmt);
    else if (auto continue_stmt = stmt.match<ContinueStmt>()) return clone_continue_stmt(*continue_stmt);
    else if (auto break_stmt = stmt.match<BreakStmt>()) return clone_break_stmt(*break_stmt);
    else if (auto expr = stmt.match<Expr>()) return clone_expr_stmt(*expr);
    else if (auto block = stmt.match<Block>()) return clone_block_stmt(*block);
    else ASSERT_UNREACHABLE;
}

VarStmt *Cloner::clone_var_stmt(const VarStmt &var_stmt) {
    return mod.create_stmt(VarStmt{
        .ast_node = var_stmt.ast_node,
        .name = var_stmt.name,
        .type = clone_expr(var_stmt.type),
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
    std::vector<IfCondBranch> cond_branches;
    cond_branches.resize(if_stmt.cond_branches.size());

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
    if (!expr) return nullptr;
    else if (auto int_literal = expr.match<IntLiteral>()) return clone_int_literal(*int_literal);
    else if (auto fp_literal = expr.match<FPLiteral>()) return clone_fp_literal(*fp_literal);
    else if (auto bool_literal = expr.match<BoolLiteral>()) return clone_bool_literal(*bool_literal);
    else if (auto char_literal = expr.match<CharLiteral>()) return clone_char_literal(*char_literal);
    else if (auto array_literal = expr.match<ArrayLiteral>()) return clone_array_literal(*array_literal);
    else if (auto string_literal = expr.match<StringLiteral>()) return clone_string_literal(*string_literal);
    else if (auto struct_literal = expr.match<StructLiteral>()) return clone_struct_literal(*struct_literal);
    else if (auto symbol_expr = expr.match<SymbolExpr>()) return clone_symbol_expr(*symbol_expr);
    else if (auto binary_expr = expr.match<BinaryExpr>()) return clone_binary_expr(*binary_expr);
    else if (auto unary_expr = expr.match<UnaryExpr>()) return clone_unary_expr(*unary_expr);
    else if (auto cast_expr = expr.match<CastExpr>()) return clone_cast_expr(*cast_expr);
    else if (auto index_expr = expr.match<IndexExpr>()) return clone_index_expr(*index_expr);
    else if (auto call_expr = expr.match<CallExpr>()) return clone_call_expr(*call_expr);
    else if (auto field_expr = expr.match<FieldExpr>()) return clone_field_expr(*field_expr);
    else if (auto range_expr = expr.match<RangeExpr>()) return clone_range_expr(*range_expr);
    else if (auto tuple_expr = expr.match<TupleExpr>()) return clone_tuple_expr(*tuple_expr);
    else if (auto primitive_type = expr.match<PrimitiveType>()) return clone_primitive_type(*primitive_type);
    else if (auto pointer_type = expr.match<PointerType>()) return clone_pointer_type(*pointer_type);
    else if (auto static_array_type = expr.match<StaticArrayType>()) return clone_static_array_type(*static_array_type);
    else if (auto func_type = expr.match<FuncType>()) return clone_func_type(*func_type);
    else if (auto ident_expr = expr.match<IdentExpr>()) return clone_ident_expr(*ident_expr);
    else if (auto star_expr = expr.match<StarExpr>()) return clone_star_expr(*star_expr);
    else if (auto bracket_expr = expr.match<BracketExpr>()) return clone_bracket_expr(*bracket_expr);
    else if (auto dot_expr = expr.match<DotExpr>()) return clone_dot_expr(*dot_expr);
    else ASSERT_UNREACHABLE;
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
    std::vector<StructLiteralEntry> entries;
    entries.resize(struct_literal.entries.size());

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

SymbolTable *Cloner::push_symbol_table(SymbolTable *parent_if_empty) {
    SymbolTable *symbol_table = mod.create_symbol_table({
        .parent = symbol_tables.empty() ? parent_if_empty : symbol_tables.top(),
        .symbols = {},
    });

    symbol_tables.push(symbol_table);
    return symbol_table;
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

        if (auto expr = node.match<sir::Expr>()) nodes[i] = clone_expr(*expr);
        else if (auto stmt = node.match<sir::Stmt>()) nodes[i] = clone_stmt(*stmt);
        else if (auto decl = node.match<sir::Decl>()) nodes[i] = clone_decl(*decl);
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
