#include "sir_generator.hpp"

#include "banjo/ast/ast_child_indices.hpp"
#include "banjo/ast/ast_node.hpp"
#include "banjo/ast/expr.hpp"
#include "banjo/sir/sir.hpp"
#include "banjo/utils/macros.hpp"

#include <string>
#include <vector>

namespace banjo {

namespace lang {

sir::Unit SIRGenerator::generate(ModuleList &mods) {
    sir::Unit sir_unit;

    for (ASTModule *mod : mods) {
        sir::Module *sir_mod = sir_unit.create_mod();
        sir_unit.mods.push_back(sir_mod);
        sir_unit.mods_by_path.insert({mod->get_path(), sir_mod});
    }

    for (ASTModule *mod : mods) {
        sir::Module *sir_mod = sir_unit.mods_by_path[mod->get_path()];
        generate_mod(mod, *sir_mod);
    }

    return sir_unit;
}

sir::Module SIRGenerator::generate(ASTModule *node) {
    sir::Module sir_mod;
    generate_mod(node, sir_mod);
    return sir_mod;
}

void SIRGenerator::regenerate_mod(sir::Unit &unit, ASTModule *mod) {
    sir::Module *sir_mod = unit.mods_by_path[mod->get_path()];
    generate_mod(mod, *sir_mod);
}

void SIRGenerator::generate_mod(ASTModule *node, sir::Module &out_sir_mod) {
    cur_sir_mod = &out_sir_mod;
    out_sir_mod.path = node->get_path();
    out_sir_mod.block = generate_decl_block(node->get_block());
}

sir::DeclBlock SIRGenerator::generate_decl_block(ASTNode *node) {
    sir::SymbolTable *symbol_table = create_symbol_table({
        .parent = scopes.empty() ? nullptr : get_scope().symbol_table,
        .symbols = {},
    });

    sir::DeclBlock sir_block{
        .ast_node = node,
        .decls = {},
        .symbol_table = symbol_table,
    };

    if (scopes.empty()) {
        scopes.push({
            .symbol_table = symbol_table,
        });
    } else {
        push_scope().symbol_table = symbol_table;
    }

    for (ASTNode *child : node->get_children()) {
        sir_block.decls.push_back(generate_decl(child));
    }

    pop_scope();

    return sir_block;
}

sir::Decl SIRGenerator::generate_decl(ASTNode *node) {
    switch (node->get_type()) {
        case AST_FUNCTION_DEFINITION: return generate_func_def(node);
        case AST_GENERIC_FUNCTION_DEFINITION: return generate_generic_func(node);
        case AST_FUNC_DECL: return generate_func_decl(node);
        case AST_NATIVE_FUNCTION_DECLARATION: return generate_native_func(node);
        case AST_CONSTANT: return generate_const(node);
        case AST_STRUCT_DEFINITION: return generate_struct(node);
        case AST_GENERIC_STRUCT_DEFINITION: return generate_generic_struct(node);
        case AST_ENUM_DEFINITION: return generate_enum(node);
        case AST_UNION: return generate_union(node);
        case AST_UNION_CASE: return generate_union_case(node);
        case AST_PROTO: return generate_proto(node);
        case AST_TYPE_ALIAS: return generate_type_alias(node);
        case AST_VAR: return generate_var_decl(node);
        case AST_NATIVE_VAR: return generate_native_var_decl(node);
        case AST_USE: return generate_use_decl(node);
        case AST_META_IF: return generate_meta_if_stmt(node, MetaBlockKind::DECL);
        case AST_IDENTIFIER: return generate_error_decl(node);
        case AST_ERROR: return generate_error_decl(node);
        case AST_COMPLETION_TOKEN: return generate_completion_token(node);
        default: ASSERT_UNREACHABLE;
    }
}

sir::Decl SIRGenerator::generate_func_def(ASTNode *node) {
    return create_decl(sir::FuncDef{
        .ast_node = node,
        .ident = generate_ident(node->get_child(FUNC_NAME)),
        .type = generate_func_type(node->get_child(FUNC_PARAMS), node->get_child(FUNC_TYPE)),
        .block = generate_block(node->get_child(FUNC_BLOCK)),
        .attrs = node->get_attribute_list() ? generate_attrs(*node->get_attribute_list()) : nullptr,
        .generic_params = {},
    });
}

sir::Decl SIRGenerator::generate_generic_func(ASTNode *node) {
    return create_decl(sir::FuncDef{
        .ast_node = node,
        .ident = generate_ident(node->get_child(GENERIC_FUNC_NAME)),
        .type = generate_func_type(node->get_child(GENERIC_FUNC_PARAMS), node->get_child(GENERIC_FUNC_TYPE)),
        .block = generate_block(node->get_child(GENERIC_FUNC_BLOCK)),
        .generic_params = generate_generic_param_list(node->get_child(GENERIC_FUNC_GENERIC_PARAMS)),
    });
}

sir::Decl SIRGenerator::generate_func_decl(ASTNode *node) {
    return create_decl(sir::FuncDecl{
        .ast_node = node,
        .ident = generate_ident(node->get_child(FUNC_NAME)),
        .type = generate_func_type(node->get_child(FUNC_PARAMS), node->get_child(FUNC_TYPE)),
    });
}

sir::Decl SIRGenerator::generate_native_func(ASTNode *node) {
    return create_decl(sir::NativeFuncDecl{
        .ast_node = node,
        .ident = generate_ident(node->get_child(FUNC_NAME)),
        .type = generate_func_type(node->get_child(FUNC_PARAMS), node->get_child(FUNC_TYPE)),
        .attrs = node->get_attribute_list() ? generate_attrs(*node->get_attribute_list()) : nullptr,
    });
}

sir::Decl SIRGenerator::generate_const(ASTNode *node) {
    return create_decl(sir::ConstDef{
        .ast_node = node,
        .ident = generate_ident(node->get_child(CONST_NAME)),
        .type = generate_expr(node->get_child(CONST_TYPE)),
        .value = generate_expr(node->get_child(CONST_VALUE)),
    });
}

sir::Decl SIRGenerator::generate_struct(ASTNode *node) {
    sir::StructDef *struct_def = create_decl(sir::StructDef{
        .ast_node = node,
        .ident = generate_ident(node->get_child(STRUCT_NAME)),
        .block = {},
        .fields = {},
        .impls = generate_expr_list(node->get_child(STRUCT_IMPL_LIST)),
        .generic_params = {},
    });

    push_scope().struct_def = struct_def;
    struct_def->block = generate_decl_block(node->get_child(STRUCT_BLOCK));
    pop_scope();

    return struct_def;
}

sir::Decl SIRGenerator::generate_generic_struct(ASTNode *node) {
    sir::StructDef *struct_def = create_decl(sir::StructDef{
        .ast_node = node,
        .ident = generate_ident(node->get_child(GENERIC_STRUCT_NAME)),
        .block = {},
        .fields = {},
        .generic_params = generate_generic_param_list(node->get_child(GENERIC_STRUCT_GENERIC_PARAMS)),
    });

    push_scope().struct_def = struct_def;
    struct_def->block = generate_decl_block(node->get_child(GENERIC_STRUCT_BLOCK));
    pop_scope();

    return struct_def;
}

sir::Decl SIRGenerator::generate_var_decl(ASTNode *node) {
    return create_decl(sir::VarDecl{
        .ast_node = node,
        .ident = generate_ident(node->get_child(VAR_NAME)),
        .type = generate_expr(node->get_child(VAR_TYPE)),
        .value = node->has_child(VAR_VALUE) ? generate_expr(node->get_child(VAR_VALUE)) : nullptr,
        .attrs = node->get_attribute_list() ? generate_attrs(*node->get_attribute_list()) : nullptr,
    });
}

sir::Decl SIRGenerator::generate_native_var_decl(ASTNode *node) {
    return create_decl(sir::NativeVarDecl{
        .ast_node = node,
        .ident = generate_ident(node->get_child(VAR_NAME)),
        .type = generate_expr(node->get_child(VAR_TYPE)),
        .attrs = node->get_attribute_list() ? generate_attrs(*node->get_attribute_list()) : nullptr,
    });
}

sir::Decl SIRGenerator::generate_enum(ASTNode *node) {
    // TODO: The parser should create a block node instead of a variant list node so this hack can be removed.
    ASTNode dummy_block_node;
    sir::DeclBlock block = generate_decl_block(&dummy_block_node);

    for (ASTNode *variant : node->get_child(ENUM_VARIANTS)->get_children()) {
        bool has_value = variant->has_child(ENUM_VARIANT_VALUE);

        block.decls.push_back(create_decl(sir::EnumVariant{
            .ast_node = variant,
            .ident = generate_ident(variant->get_child(ENUM_VARIANT_NAME)),
            .value = has_value ? generate_expr(variant->get_child(ENUM_VARIANT_VALUE)) : nullptr,
        }));
    }

    return create_decl(sir::EnumDef{
        .ast_node = node,
        .ident = generate_ident(node->get_child(ENUM_NAME)),
        .block = block,
    });
}

sir::Decl SIRGenerator::generate_union(ASTNode *node) {
    return create_decl(sir::UnionDef{
        .ast_node = node,
        .ident = generate_ident(node->get_child(UNION_NAME)),
        .block = generate_decl_block(node->get_child(UNION_BLOCK)),
        .cases = {},
    });
}

sir::Decl SIRGenerator::generate_union_case(ASTNode *node) {
    return create_decl(sir::UnionCase{
        .ast_node = node,
        .ident = generate_ident(node->get_child(UNION_CASE_NAME)),
        .fields = generate_union_case_fields(node->get_child(UNION_CASE_FIELDS)),
    });
}

sir::Decl SIRGenerator::generate_proto(ASTNode *node) {
    return create_decl(sir::ProtoDef{
        .ast_node = node,
        .ident = generate_ident(node->get_child(PROTO_NAME)),
        .block = generate_decl_block(node->get_child(PROTO_BLOCK)),
        .func_decls = {},
    });
}

sir::Decl SIRGenerator::generate_type_alias(ASTNode *node) {
    return create_decl(sir::TypeAlias{
        .ast_node = node,
        .ident = generate_ident(node->get_child(TYPE_ALIAS_NAME)),
        .type = generate_expr(node->get_child(TYPE_ALIAS_UNDERLYING_TYPE)),
    });
}

sir::Decl SIRGenerator::generate_use_decl(ASTNode *node) {
    return create_decl(sir::UseDecl{
        .ast_node = node,
        .root_item = generate_use_item(node->get_child()),
    });
}

sir::Decl SIRGenerator::generate_error_decl(ASTNode *node) {
    return create_decl(sir::Error{
        .ast_node = node,
    });
}

sir::Ident SIRGenerator::generate_ident(ASTNode *node) {
    return sir::Ident{
        .ast_node = node,
        .value = node->get_value(),
    };
}

sir::Block SIRGenerator::generate_block(ASTNode *node) {
    sir::SymbolTable *symbol_table = create_symbol_table({
        .parent = get_scope().symbol_table,
        .symbols = {},
    });

    sir::Block sir_block{
        .ast_node = node,
        .stmts = {},
        .symbol_table = symbol_table,
    };

    push_scope().symbol_table = symbol_table;

    for (ASTNode *child : node->get_children()) {
        sir_block.stmts.push_back(generate_stmt(child));
    }

    pop_scope();

    return sir_block;
}

sir::MetaBlock SIRGenerator::generate_meta_block(ASTNode *node, MetaBlockKind kind) {
    sir::MetaBlock sir_meta_block{
        .ast_node = node,
        .nodes = {},
    };

    for (ASTNode *child : node->get_children()) {
        switch (kind) {
            case MetaBlockKind::DECL: sir_meta_block.nodes.push_back(generate_decl(child)); break;
            case MetaBlockKind::STMT: sir_meta_block.nodes.push_back(generate_stmt(child)); break;
        }
    }

    return sir_meta_block;
}

sir::Stmt SIRGenerator::generate_stmt(ASTNode *node) {
    switch (node->get_type()) {
        case AST_VAR: return generate_var_stmt(node);
        case AST_ASSIGNMENT: return generate_assign_stmt(node);
        case AST_ADD_ASSIGN: return generate_comp_assign_stmt(node, sir::BinaryOp::ADD);
        case AST_SUB_ASSIGN: return generate_comp_assign_stmt(node, sir::BinaryOp::SUB);
        case AST_MUL_ASSIGN: return generate_comp_assign_stmt(node, sir::BinaryOp::MUL);
        case AST_DIV_ASSIGN: return generate_comp_assign_stmt(node, sir::BinaryOp::DIV);
        case AST_MOD_ASSIGN: return generate_comp_assign_stmt(node, sir::BinaryOp::MOD);
        case AST_BIT_AND_ASSIGN: return generate_comp_assign_stmt(node, sir::BinaryOp::BIT_AND);
        case AST_BIT_OR_ASSIGN: return generate_comp_assign_stmt(node, sir::BinaryOp::BIT_OR);
        case AST_BIT_XOR_ASSIGN: return generate_comp_assign_stmt(node, sir::BinaryOp::BIT_XOR);
        case AST_SHL_ASSIGN: return generate_comp_assign_stmt(node, sir::BinaryOp::SHL);
        case AST_SHR_ASSIGN: return generate_comp_assign_stmt(node, sir::BinaryOp::SHR);
        case AST_IMPLICIT_TYPE_VAR: return generate_typeless_var_stmt(node);
        case AST_FUNCTION_RETURN: return generate_return_stmt(node);
        case AST_IF_CHAIN: return generate_if_stmt(node);
        case AST_SWITCH: return generate_switch_stmt(node);
        case AST_TRY: return generate_try_stmt(node);
        case AST_WHILE: return generate_while_stmt(node);
        case AST_FOR: return generate_for_stmt(node);
        case AST_CONTINUE: return generate_continue_stmt(node);
        case AST_BREAK: return generate_break_stmt(node);
        case AST_META_IF: return generate_meta_if_stmt(node, MetaBlockKind::STMT);
        case AST_META_FOR: return generate_meta_for_stmt(node, MetaBlockKind::STMT);
        case AST_FUNCTION_CALL: return create_stmt(generate_expr(node));
        case AST_IDENTIFIER: return create_stmt(generate_expr(node));
        case AST_DOT_OPERATOR: return create_stmt(generate_expr(node));
        case AST_BLOCK: return create_stmt(generate_block(node));
        case AST_ERROR: return generate_error_stmt(node);
        case AST_COMPLETION_TOKEN: return create_stmt(generate_expr(node));
        default: ASSERT_UNREACHABLE;
    }
}

sir::Stmt SIRGenerator::generate_var_stmt(ASTNode *node) {
    return create_stmt(sir::VarStmt{
        .ast_node = node,
        .local = generate_local(node->get_child(VAR_NAME), node->get_child(VAR_TYPE)),
        .value = node->has_child(VAR_VALUE) ? generate_expr(node->get_child(VAR_VALUE)) : nullptr,
    });
}

sir::Stmt SIRGenerator::generate_typeless_var_stmt(ASTNode *node) {
    return create_stmt(sir::VarStmt{
        .ast_node = node,
        .local = generate_local(node->get_child(TYPE_INFERRED_VAR_NAME), nullptr),
        .value = generate_expr(node->get_child(TYPE_INFERRED_VAR_VALUE)),
    });
}

sir::Stmt SIRGenerator::generate_assign_stmt(ASTNode *node) {
    return create_stmt(sir::AssignStmt{
        .ast_node = node,
        .lhs = generate_expr(node->get_child(ASSIGN_LOCATION)),
        .rhs = generate_expr(node->get_child(ASSIGN_VALUE)),
    });
}

sir::Stmt SIRGenerator::generate_comp_assign_stmt(ASTNode *node, sir::BinaryOp op) {
    return create_stmt(sir::CompAssignStmt{
        .ast_node = node,
        .op = op,
        .lhs = generate_expr(node->get_child(ASSIGN_LOCATION)),
        .rhs = generate_expr(node->get_child(ASSIGN_VALUE)),
    });
}

sir::Stmt SIRGenerator::generate_return_stmt(ASTNode *node) {
    return create_stmt(sir::ReturnStmt{
        .ast_node = node,
        .value = node->has_child(0) ? generate_expr(node->get_child()) : nullptr,
    });
}

sir::Stmt SIRGenerator::generate_if_stmt(ASTNode *node) {
    sir::IfStmt sir_if_stmt{
        .ast_node = node,
        .cond_branches = {},
        .else_branch = {},
    };

    for (ASTNode *child : node->get_children()) {
        if (child->get_type() == AST_IF || child->get_type() == AST_ELSE_IF) {
            sir_if_stmt.cond_branches.push_back({
                .ast_node = child,
                .condition = generate_expr(child->get_child(IF_CONDITION)),
                .block = generate_block(child->get_child(IF_BLOCK)),
            });
        } else if (child->get_type() == AST_ELSE) {
            sir_if_stmt.else_branch = {
                .ast_node = child,
                .block = generate_block(child->get_child()),
            };
        } else {
            ASSERT_UNREACHABLE;
        }
    }

    return create_stmt(sir_if_stmt);
}

sir::Stmt SIRGenerator::generate_switch_stmt(ASTNode *node) {
    sir::SwitchStmt sir_switch_stmt{
        .ast_node = node,
        .value = generate_expr(node->get_child(SWITCH_VALUE)),
        .case_branches = {},
    };

    for (ASTNode *child : node->get_child(SWITCH_CASES)->get_children()) {
        sir_switch_stmt.case_branches.push_back(sir::SwitchCaseBranch{
            .ast_node = child,
            .local = generate_local(child->get_child(0), child->get_child(1)),
            .block = generate_block(child->get_child(2)),
        });
    }

    return create_stmt(sir_switch_stmt);
}

sir::Stmt SIRGenerator::generate_try_stmt(ASTNode *node) {
    sir::TryStmt sir_try_stmt{
        .ast_node = node,
    };

    for (ASTNode *child : node->get_children()) {
        if (child->get_type() == AST_TRY_SUCCESS_CASE) {
            sir_try_stmt.success_branch = sir::TrySuccessBranch{
                .ast_node = child,
                .ident = generate_ident(child->get_child(0)),
                .expr = generate_expr(child->get_child(1)),
                .block = generate_block(child->get_child(2)),
            };
        } else if (child->get_type() == AST_TRY_ERROR_CASE) {
            sir_try_stmt.except_branch = sir::TryExceptBranch{
                .ast_node = child,
                .ident = generate_ident(child->get_child(0)),
                .type = generate_expr(child->get_child(1)),
                .block = generate_block(child->get_child(2)),
            };
        } else if (child->get_type() == AST_TRY_ELSE_CASE) {
            sir_try_stmt.else_branch = sir::TryElseBranch{
                .ast_node = child,
                .block = generate_block(child->get_child(0)),
            };
        } else {
            ASSERT_UNREACHABLE;
        }
    }

    return create_stmt(sir_try_stmt);
}

sir::Stmt SIRGenerator::generate_while_stmt(ASTNode *node) {
    return create_stmt(sir::WhileStmt{
        .ast_node = node,
        .condition = generate_expr(node->get_child(WHILE_CONDITION)),
        .block = generate_block(node->get_child(WHILE_BLOCK)),
    });
}

sir::Stmt SIRGenerator::generate_for_stmt(ASTNode *node) {
    return create_stmt(sir::ForStmt{
        .ast_node = node,
        .ident = generate_ident(node->get_child(FOR_VAR)),
        .range = generate_expr(node->get_child(FOR_EXPR)),
        .block = generate_block(node->get_child(FOR_BLOCK)),
    });
}

sir::Stmt SIRGenerator::generate_continue_stmt(ASTNode *node) {
    return create_stmt(sir::ContinueStmt{
        .ast_node = node,
    });
}

sir::Stmt SIRGenerator::generate_break_stmt(ASTNode *node) {
    return create_stmt(sir::BreakStmt{
        .ast_node = node,
    });
}

sir::MetaIfStmt *SIRGenerator::generate_meta_if_stmt(ASTNode *node, MetaBlockKind kind) {
    sir::MetaIfStmt sir_meta_if_stmt{
        .ast_node = node,
        .cond_branches = {},
        .else_branch = {},
    };

    for (ASTNode *child : node->get_children()) {
        if (child->get_type() == AST_META_IF_CONDITION) {
            sir_meta_if_stmt.cond_branches.push_back({
                .ast_node = child,
                .condition = generate_expr(child->get_child(IF_CONDITION)),
                .block = generate_meta_block(child->get_child(IF_BLOCK), kind),
            });
        } else if (child->get_type() == AST_META_ELSE) {
            sir_meta_if_stmt.else_branch = {
                .ast_node = child,
                .block = generate_meta_block(child->get_child(), kind),
            };
        } else {
            ASSERT_UNREACHABLE;
        }
    }

    return create_stmt(sir_meta_if_stmt);
}

sir::MetaForStmt *SIRGenerator::generate_meta_for_stmt(ASTNode *node, MetaBlockKind kind) {
    return create_stmt(sir::MetaForStmt{
        .ast_node = node,
        .ident = generate_ident(node->get_child(0)),
        .range = generate_expr(node->get_child(1)),
        .block = generate_meta_block(node->get_child(2), kind),
    });
}

sir::Stmt SIRGenerator::generate_error_stmt(ASTNode *node) {
    return create_stmt(sir::Error{
        .ast_node = node,
    });
}

sir::Expr SIRGenerator::generate_expr(ASTNode *node) {
    switch (node->get_type()) {
        case AST_INT_LITERAL: return generate_int_literal(node);
        case AST_FLOAT_LITERAL: return generate_fp_literal(node);
        case AST_FALSE: return generate_bool_literal(node, false);
        case AST_TRUE: return generate_bool_literal(node, true);
        case AST_CHAR_LITERAL: return generate_char_literal(node);
        case AST_NULL: return generate_null_literal(node);
        case AST_NONE: return generate_none_literal(node);
        case AST_UNDEFINED: return generate_undefined_literal(node);
        case AST_ARRAY_EXPR: return generate_array_literal(node);
        case AST_STRING_LITERAL: return generate_string_literal(node);
        case AST_STRUCT_INSTANTIATION: return generate_struct_literal(node);
        case AST_ANON_STRUCT_LITERAL: return generate_typeless_struct_literal(node);
        case AST_MAP_EXPR: return generate_map_literal(node);
        case AST_CLOSURE: return generate_closure_literal(node);
        case AST_IDENTIFIER: return generate_ident_expr(node);
        case AST_SELF: return generate_self(node);
        case AST_OPERATOR_ADD: return generate_binary_expr(node, sir::BinaryOp::ADD);
        case AST_OPERATOR_SUB: return generate_binary_expr(node, sir::BinaryOp::SUB);
        case AST_OPERATOR_MUL: return generate_binary_expr(node, sir::BinaryOp::MUL);
        case AST_OPERATOR_DIV: return generate_binary_expr(node, sir::BinaryOp::DIV);
        case AST_OPERATOR_MOD: return generate_binary_expr(node, sir::BinaryOp::MOD);
        case AST_OPERATOR_BIT_AND: return generate_binary_expr(node, sir::BinaryOp::BIT_AND);
        case AST_OPERATOR_BIT_OR: return generate_binary_expr(node, sir::BinaryOp::BIT_OR);
        case AST_OPERATOR_BIT_XOR: return generate_binary_expr(node, sir::BinaryOp::BIT_XOR);
        case AST_OPERATOR_SHL: return generate_binary_expr(node, sir::BinaryOp::SHL);
        case AST_OPERATOR_SHR: return generate_binary_expr(node, sir::BinaryOp::SHR);
        case AST_OPERATOR_EQ: return generate_binary_expr(node, sir::BinaryOp::EQ);
        case AST_OPERATOR_NE: return generate_binary_expr(node, sir::BinaryOp::NE);
        case AST_OPERATOR_GT: return generate_binary_expr(node, sir::BinaryOp::GT);
        case AST_OPERATOR_LT: return generate_binary_expr(node, sir::BinaryOp::LT);
        case AST_OPERATOR_GE: return generate_binary_expr(node, sir::BinaryOp::GE);
        case AST_OPERATOR_LE: return generate_binary_expr(node, sir::BinaryOp::LE);
        case AST_OPERATOR_AND: return generate_binary_expr(node, sir::BinaryOp::AND);
        case AST_OPERATOR_OR: return generate_binary_expr(node, sir::BinaryOp::OR);
        case AST_OPERATOR_NEG: return generate_unary_expr(node, sir::UnaryOp::NEG);
        case AST_OPERATOR_REF: return generate_unary_expr(node, sir::UnaryOp::REF);
        case AST_STAR_EXPR: return generate_star_expr(node);
        case AST_OPERATOR_NOT: return generate_unary_expr(node, sir::UnaryOp::NOT);
        case AST_CAST: return generate_cast_expr(node);
        case AST_FUNCTION_CALL: return generate_call_expr(node);
        case AST_DOT_OPERATOR: return generate_dot_expr(node);
        case AST_ARRAY_ACCESS: return generate_bracket_expr(node);
        case AST_RANGE: return generate_range_expr(node);
        case AST_TUPLE_EXPR: return generate_tuple_expr(node);
        case AST_I8: return generate_primitive_type(node, sir::Primitive::I8);
        case AST_I16: return generate_primitive_type(node, sir::Primitive::I16);
        case AST_I32: return generate_primitive_type(node, sir::Primitive::I32);
        case AST_I64: return generate_primitive_type(node, sir::Primitive::I64);
        case AST_U8: return generate_primitive_type(node, sir::Primitive::U8);
        case AST_U16: return generate_primitive_type(node, sir::Primitive::U16);
        case AST_U32: return generate_primitive_type(node, sir::Primitive::U32);
        case AST_U64: return generate_primitive_type(node, sir::Primitive::U64);
        case AST_F32: return generate_primitive_type(node, sir::Primitive::F32);
        case AST_F64: return generate_primitive_type(node, sir::Primitive::F64);
        case AST_USIZE: return generate_primitive_type(node, sir::Primitive::USIZE);
        case AST_BOOL: return generate_primitive_type(node, sir::Primitive::BOOL);
        case AST_ADDR: return generate_primitive_type(node, sir::Primitive::ADDR);
        case AST_VOID: return generate_primitive_type(node, sir::Primitive::VOID);
        case AST_STATIC_ARRAY_TYPE: return generate_static_array_type(node);
        case AST_FUNCTION_DATA_TYPE: return generate_func_type(node);
        case AST_OPTIONAL_DATA_TYPE: return generate_optional_type(node);
        case AST_RESULT_TYPE: return generate_result_type(node);
        case AST_CLOSURE_TYPE: return generate_closure_type(node);
        case AST_META_EXPR: return generate_meta_access(node);
        case AST_ERROR: return generate_error_expr(node);
        case AST_COMPLETION_TOKEN: return generate_completion_token(node);
        default: ASSERT_UNREACHABLE;
    }
}

sir::Expr SIRGenerator::generate_int_literal(ASTNode *node) {
    return create_expr(sir::IntLiteral{
        .ast_node = node,
        .type = nullptr,
        .value = node->as<IntLiteral>()->get_value(),
    });
}

sir::Expr SIRGenerator::generate_fp_literal(ASTNode *node) {
    return create_expr(sir::FPLiteral{
        .ast_node = node,
        .type = nullptr,
        .value = std::stod(node->get_value()),
    });
}

sir::Expr SIRGenerator::generate_bool_literal(ASTNode *node, bool value) {
    return create_expr(sir::BoolLiteral{
        .ast_node = node,
        .type = nullptr,
        .value = value,
    });
}

sir::Expr SIRGenerator::generate_char_literal(ASTNode *node) {
    unsigned index = 0;
    char value = decode_char(node->get_value(), index);

    return create_expr(sir::CharLiteral{
        .ast_node = node,
        .type = nullptr,
        .value = value,
    });
}

sir::Expr SIRGenerator::generate_null_literal(ASTNode *node) {
    return create_expr(sir::NullLiteral{
        .ast_node = node,
        .type = nullptr,
    });
}

sir::Expr SIRGenerator::generate_none_literal(ASTNode *node) {
    return create_expr(sir::NoneLiteral{
        .ast_node = node,
        .type = nullptr,
    });
}

sir::Expr SIRGenerator::generate_undefined_literal(ASTNode *node) {
    return create_expr(sir::UndefinedLiteral{
        .ast_node = node,
        .type = nullptr,
    });
}

sir::Expr SIRGenerator::generate_array_literal(ASTNode *node) {
    std::vector<sir::Expr> values(node->get_children().size());

    for (unsigned i = 0; i < node->get_children().size(); i++) {
        values[i] = generate_expr(node->get_child(i));
    }

    return create_expr(sir::ArrayLiteral{
        .ast_node = node,
        .type = nullptr,
        .values = values,
    });
}

sir::Expr SIRGenerator::generate_string_literal(ASTNode *node) {
    unsigned index = 0;
    std::string value = "";

    while (index < node->get_value().length()) {
        value += decode_char(node->get_value(), index);
    }

    return create_expr(sir::StringLiteral{
        .ast_node = node,
        .type = nullptr,
        .value = value,
    });
}

sir::Expr SIRGenerator::generate_struct_literal(ASTNode *node) {
    return create_expr(sir::StructLiteral{
        .ast_node = node,
        .type = generate_expr(node->get_child(STRUCT_LITERAL_NAME)),
        .entries = generate_struct_literal_entries(node->get_child(STRUCT_LITERAL_VALUES)),
    });
}

sir::Expr SIRGenerator::generate_typeless_struct_literal(ASTNode *node) {
    return create_expr(sir::StructLiteral{
        .ast_node = node,
        .type = nullptr,
        .entries = generate_struct_literal_entries(node->get_child()),
    });
}

sir::Expr SIRGenerator::generate_map_literal(ASTNode *node) {
    return create_expr(sir::MapLiteral{
        .ast_node = node,
        .type = nullptr,
        .entries = generate_map_literal_entries(node),
    });
}

sir::Expr SIRGenerator::generate_closure_literal(ASTNode *node) {
    return create_expr(sir::ClosureLiteral{
        .ast_node = node,
        .type = nullptr,
        .func_type = generate_func_type(node->get_child(CLOSURE_PARAMS), node->get_child(CLOSURE_RETURN_TYPE)),
        .block = generate_block(node->get_child(CLOSURE_BLOCK)),
    });
}

sir::Expr SIRGenerator::generate_ident_expr(ASTNode *node) {
    return create_expr(sir::IdentExpr{
        .ast_node = node,
        .value = node->get_value(),
    });
}

sir::Expr SIRGenerator::generate_self(ASTNode *node) {
    return create_expr(sir::IdentExpr{
        .ast_node = node,
        .value = "self",
    });
}

sir::Expr SIRGenerator::generate_binary_expr(ASTNode *node, sir::BinaryOp op) {
    return create_expr(sir::BinaryExpr{
        .ast_node = node,
        .type = nullptr,
        .op = op,
        .lhs = generate_expr(node->get_child(0)),
        .rhs = generate_expr(node->get_child(1)),
    });
}

sir::Expr SIRGenerator::generate_unary_expr(ASTNode *node, sir::UnaryOp op) {
    return create_expr(sir::UnaryExpr{
        .ast_node = node,
        .type = nullptr,
        .op = op,
        .value = generate_expr(node->get_child()),
    });
}

sir::Expr SIRGenerator::generate_cast_expr(ASTNode *node) {
    return create_expr(sir::CastExpr{
        .ast_node = node,
        .type = generate_expr(node->get_child(1)),
        .value = generate_expr(node->get_child(0)),
    });
}

sir::Expr SIRGenerator::generate_call_expr(ASTNode *node) {
    sir::Expr callee = generate_expr(node->get_child(0));
    std::vector<sir::Expr> args = generate_expr_list(node->get_child(1));

    if (callee.is<sir::MetaFieldExpr>()) {
        return create_expr(sir::MetaCallExpr{
            .ast_node = node,
            .callee = callee,
            .args = args,
        });
    } else {
        return create_expr(sir::CallExpr{
            .ast_node = node,
            .callee = callee,
            .args = args,
        });
    }
}

sir::Expr SIRGenerator::generate_dot_expr(ASTNode *node) {
    ASTNode *lhs_node = node->get_child(0);
    ASTNode *rhs_node = node->get_child(1);

    sir::Expr lhs = generate_expr(lhs_node);
    sir::Ident rhs;

    if (rhs_node->get_type() == AST_IDENTIFIER) {
        rhs = generate_ident(rhs_node);
    } else if (rhs_node->get_type() == AST_INT_LITERAL) {
        rhs = {
            .ast_node = rhs_node,
            .value = rhs_node->as<IntLiteral>()->get_value().to_string(),
        };
    } else if (rhs_node->get_type() == AST_COMPLETION_TOKEN) {
        rhs = {
            .ast_node = rhs_node,
            .value = "[completion]",
        };
    } else if (rhs_node->get_type() == AST_ERROR) {
        rhs = {
            .ast_node = rhs_node,
            .value = "[error]",
        };
    }

    if (lhs.is<sir::MetaAccess>()) {
        return create_expr(sir::MetaFieldExpr{
            .ast_node = node,
            .base = lhs,
            .field = rhs,
        });
    } else {
        return create_expr(sir::DotExpr{
            .ast_node = node,
            .lhs = lhs,
            .rhs = rhs,
        });
    }
}

sir::Expr SIRGenerator::generate_bracket_expr(ASTNode *node) {
    return create_expr(sir::BracketExpr{
        .ast_node = node,
        .lhs = generate_expr(node->get_child(0)),
        .rhs = generate_expr_list(node->get_child(1)),
    });
}

sir::Expr SIRGenerator::generate_range_expr(ASTNode *node) {
    return create_expr(sir::RangeExpr{
        .ast_node = node,
        .lhs = generate_expr(node->get_child(0)),
        .rhs = generate_expr(node->get_child(1)),
    });
}

sir::Expr SIRGenerator::generate_tuple_expr(ASTNode *node) {
    return create_expr(sir::TupleExpr{
        .ast_node = node,
        .type = nullptr,
        .exprs = generate_expr_list(node),
    });
}

sir::Expr SIRGenerator::generate_star_expr(ASTNode *node) {
    return create_expr(sir::StarExpr{
        .ast_node = node,
        .value = generate_expr(node->get_child()),
    });
}

sir::Expr SIRGenerator::generate_primitive_type(ASTNode *node, sir::Primitive primitive) {
    return create_expr(sir::PrimitiveType{
        .ast_node = node,
        .primitive = primitive,
    });
}

sir::Expr SIRGenerator::generate_static_array_type(ASTNode *node) {
    return create_expr(sir::StaticArrayType{
        .ast_node = node,
        .base_type = generate_expr(node->get_child(0)),
        .length = generate_expr(node->get_child(1)),
    });
}

sir::Expr SIRGenerator::generate_func_type(ASTNode *node) {
    return create_expr(generate_func_type(node->get_child(FUNC_TYPE_PARAMS), node->get_child(FUNC_TYPE_RETURN_TYPE)));
}

sir::Expr SIRGenerator::generate_optional_type(ASTNode *node) {
    return create_expr(sir::OptionalType{
        .ast_node = node,
        .base_type = generate_expr(node->get_child()),
    });
}

sir::Expr SIRGenerator::generate_result_type(ASTNode *node) {
    return create_expr(sir::ResultType{
        .ast_node = node,
        .value_type = generate_expr(node->get_child(0)),
        .error_type = generate_expr(node->get_child(1)),
    });
}

sir::Expr SIRGenerator::generate_closure_type(ASTNode *node) {
    return create_expr(sir::ClosureType{
        .ast_node = nullptr,
        .func_type =
            generate_func_type(node->get_child(CLOSURE_TYPE_PARAMS), node->get_child(CLOSURE_TYPE_RETURN_TYPE)),
        .underlying_struct = nullptr,
    });
}

sir::Expr SIRGenerator::generate_meta_access(ASTNode *node) {
    return create_expr(sir::MetaAccess{
        .ast_node = node,
        .expr = generate_expr(node->get_child()),
    });
}

sir::Expr SIRGenerator::generate_error_expr(ASTNode *node) {
    return create_expr(sir::Error{
        .ast_node = node,
    });
}

sir::FuncType SIRGenerator::generate_func_type(ASTNode *params_node, ASTNode *return_node) {
    std::vector<sir::Param> sir_params(params_node->get_children().size());

    for (unsigned i = 0; i < params_node->get_children().size(); i++) {
        sir_params[i] = generate_param(params_node->get_child(i));
    }

    return {
        .params = sir_params,
        .return_type = generate_expr(return_node),
    };
}

sir::Param SIRGenerator::generate_param(ASTNode *node) {
    ASTNodeType ident_type = node->get_child(PARAM_NAME)->get_type();

    if (ident_type == AST_IDENTIFIER) {
        return {
            .ast_node = node,
            .name = generate_ident(node->get_child(PARAM_NAME)),
            .type = generate_expr(node->get_child(PARAM_TYPE)),
            .attrs = node->get_attribute_list() ? generate_attrs(*node->get_attribute_list()) : nullptr,
        };
    } else if (ident_type == AST_SELF) {
        sir::Ident sir_ident{
            .ast_node = nullptr,
            .value = "self",
        };

        return {
            .ast_node = node,
            .name = sir_ident,
            .type = nullptr,
            .attrs = node->get_attribute_list() ? generate_attrs(*node->get_attribute_list()) : nullptr,
        };
    } else {
        ASSERT_UNREACHABLE;
    };
}

std::vector<sir::GenericParam> SIRGenerator::generate_generic_param_list(ASTNode *node) {
    std::vector<sir::GenericParam> sir_generic_params(node->get_children().size());

    for (unsigned i = 0; i < node->get_children().size(); i++) {
        ASTNode *child = node->get_child(i);

        sir::GenericParamKind kind = sir::GenericParamKind::TYPE;

        if (child->has_child(GENERIC_PARAM_TYPE)) {
            if (child->get_child(GENERIC_PARAM_TYPE)->get_type() == AST_PARAM_SEQUENCE_TYPE) {
                kind = sir::GenericParamKind::SEQUENCE;
            }
        }

        sir_generic_params[i] = sir::GenericParam{
            .ast_node = child,
            .ident = generate_ident(child->get_child(GENERIC_PARAM_NAME)),
            .kind = kind,
        };
    }

    return sir_generic_params;
}

sir::Local SIRGenerator::generate_local(ASTNode *ident_node, ASTNode *type_node) {
    return sir::Local{
        .name = generate_ident(ident_node),
        .type = type_node ? generate_expr(type_node) : nullptr,
    };
}

std::vector<sir::Expr> SIRGenerator::generate_expr_list(ASTNode *node) {
    std::vector<sir::Expr> exprs(node->get_children().size());

    for (unsigned i = 0; i < node->get_children().size(); i++) {
        exprs[i] = generate_expr(node->get_child(i));
    }

    return exprs;
}

std::vector<sir::StructLiteralEntry> SIRGenerator::generate_struct_literal_entries(ASTNode *node) {
    std::vector<sir::StructLiteralEntry> entries(node->get_children().size());

    for (unsigned i = 0; i < node->get_children().size(); i++) {
        ASTNode *child = node->get_child(i);

        entries[i] = sir::StructLiteralEntry{
            .ident = generate_ident(child->get_child(STRUCT_FIELD_VALUE_NAME)),
            .value = generate_expr(child->get_child(STRUCT_FIELD_VALUE_VALUE)),
            .field = nullptr,
        };
    }

    return entries;
}

std::vector<sir::MapLiteralEntry> SIRGenerator::generate_map_literal_entries(ASTNode *node) {
    std::vector<sir::MapLiteralEntry> entries(node->get_children().size());

    for (unsigned i = 0; i < node->get_children().size(); i++) {
        ASTNode *child = node->get_child(i);

        entries[i] = sir::MapLiteralEntry{
            .key = generate_expr(child->get_child(MAP_LITERAL_PAIR_KEY)),
            .value = generate_expr(child->get_child(MAP_LITERAL_PAIR_VALUE)),
        };
    }

    return entries;
}

std::vector<sir::UnionCaseField> SIRGenerator::generate_union_case_fields(ASTNode *node) {
    std::vector<sir::UnionCaseField> fields(node->get_children().size());

    for (unsigned i = 0; i < node->get_children().size(); i++) {
        ASTNode *child = node->get_child(i);

        fields[i] = sir::UnionCaseField{
            .ast_node = child,
            .ident = generate_ident(child->get_child(0)),
            .type = generate_expr(child->get_child(1)),
        };
    }

    return fields;
}

sir::Attributes *SIRGenerator::generate_attrs(const AttributeList &ast_attrs) {
    sir::Attributes sir_attrs;

    for (const Attribute &ast_attr : ast_attrs.get_attributes()) {
        if (ast_attr.get_name() == "exposed") sir_attrs.exposed = true;
        else if (ast_attr.get_name() == "dllexport") sir_attrs.dllexport = true;
        else if (ast_attr.get_name() == "test") sir_attrs.test = true;
        else if (ast_attr.get_name() == "link_name") sir_attrs.link_name = ast_attr.get_value();
        else if (ast_attr.get_name() == "unmanaged") sir_attrs.unmanaged = true;
        else if (ast_attr.get_name() == "byval") sir_attrs.byval = true;
        else ASSERT_UNREACHABLE;
    }

    return create_attrs(sir_attrs);
}

sir::CompletionToken *SIRGenerator::generate_completion_token(ASTNode *node) {
    return create_expr(sir::CompletionToken{
        .ast_node = node,
    });
}

char SIRGenerator::decode_char(const std::string &value, unsigned &index) {
    char c = value[index++];

    if (c == '\\') {
        c = value[index++];

        if (c == 'n') return 0x0A;
        else if (c == 'r') return 0x0D;
        else if (c == 't') return 0x09;
        else if (c == '0') return 0x00;
        else if (c == '\\') return '\\';
        else if (c == 'x') {
            index += 2;
            return (char)std::stoi(value.substr(index - 2, 2), nullptr, 16);
        }
    }

    return c;
}

sir::UseItem SIRGenerator::generate_use_item(ASTNode *node) {
    switch (node->get_type()) {
        case AST_IDENTIFIER: return generate_use_ident(node);
        case AST_USE_REBINDING: return generate_use_rebind(node);
        case AST_DOT_OPERATOR: return generate_use_dot_expr(node);
        case AST_USE_TREE_LIST: return generate_use_list(node);
        default: ASSERT_UNREACHABLE;
    }
}

sir::UseItem SIRGenerator::generate_use_ident(ASTNode *node) {
    return create_use_item(sir::UseIdent{
        .ident = generate_ident(node),
        .symbol = nullptr,
    });
}

sir::UseItem SIRGenerator::generate_use_rebind(ASTNode *node) {
    return create_use_item(sir::UseRebind{
        .ast_node = node,
        .target_ident = generate_ident(node->get_child(USE_REBINDING_TARGET)),
        .local_ident = generate_ident(node->get_child(USE_REBINDING_LOCAL)),
        .symbol = nullptr,
    });
}

sir::UseItem SIRGenerator::generate_use_dot_expr(ASTNode *node) {
    return create_use_item(sir::UseDotExpr{
        .ast_node = node,
        .lhs = generate_use_item(node->get_child(0)),
        .rhs = generate_use_item(node->get_child(1)),
    });
}

sir::UseItem SIRGenerator::generate_use_list(ASTNode *node) {
    std::vector<sir::UseItem> items;
    items.reserve(node->get_children().size());

    for (ASTNode *child : node->get_children()) {
        items.push_back(generate_use_item(child));
    }

    return create_use_item(sir::UseList{
        .ast_node = node,
        .items = items,
    });
}

} // namespace lang

} // namespace banjo
