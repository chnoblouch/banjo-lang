#include "sir_generator.hpp"

#include "banjo/ast/ast_child_indices.hpp"
#include "banjo/ast/ast_node.hpp"
#include "banjo/ast/expr.hpp"
#include "banjo/utils/macros.hpp"
#include "sir.hpp"

#include <string>
#include <utility>
#include <vector>

namespace banjo {

namespace lang {

sir::Unit SIRGenerator::generate(ModuleList &mods) {
    for (ASTModule *mod : mods) {
        sir::Module *sir_mod = sir_unit.create_mod();
        sir_mod->path = mod->get_path();

        sir_unit.mods.push_back(sir_mod);
        sir_unit.mods_by_path.insert({mod->get_path(), sir_mod});
        mod_map.insert({mod, sir_mod});
    }

    for (ASTModule *mod : mods) {
        sir::Module *sir_mod = mod_map[mod];

        for (ASTModule *sub_mod : mod->get_sub_mods()) {
            sir_mod->sub_mods.push_back(mod_map[sub_mod]);
        }
    }

    for (ASTModule *mod : mods) {
        generate_mod(mod);
    }

    return std::move(sir_unit);
}

sir::Module *SIRGenerator::generate_mod(ASTModule *node) {
    sir::Module *sir_mod = mod_map[node];
    cur_sir_mod = sir_mod;
    sir_mod->block = generate_decl_block(node->get_block());
    return sir_mod;
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
        case AST_FUNCTION_DEFINITION: return generate_func(node);
        case AST_GENERIC_FUNCTION_DEFINITION: return generate_generic_func(node);
        case AST_NATIVE_FUNCTION_DECLARATION: return generate_native_func(node);
        case AST_CONSTANT: return generate_const(node);
        case AST_STRUCT_DEFINITION: return generate_struct(node);
        case AST_GENERIC_STRUCT_DEFINITION: return generate_generic_struct(node);
        case AST_ENUM_DEFINITION: return generate_enum(node);
        case AST_VAR: return generate_var_decl(node);
        case AST_USE: return generate_use_decl(node);
        case AST_META_IF: return generate_meta_if_stmt(node);
        default: ASSERT_UNREACHABLE;
    }
}

sir::Decl SIRGenerator::generate_func(ASTNode *node) {
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

sir::Decl SIRGenerator::generate_var_decl(ASTNode *node) {
    return create_decl(sir::VarDecl{
        .ast_node = node,
        .ident = generate_ident(node->get_child(VAR_NAME)),
        .type = generate_expr(node->get_child(VAR_TYPE)),
    });
}

sir::Decl SIRGenerator::generate_use_decl(ASTNode *node) {
    return create_decl(sir::UseDecl{
        .ast_node = node,
        .root_item = generate_use_item(node->get_child()),
    });
}

sir::Ident SIRGenerator::generate_ident(ASTNode *node) {
    return {
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

sir::MetaBlock SIRGenerator::generate_meta_block(ASTNode *node) {
    sir::MetaBlock sir_meta_block{
        .ast_node = node,
        .nodes = {},
    };

    for (ASTNode *child : node->get_children()) {
        sir_meta_block.nodes.push_back(generate_decl(child));
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
        case AST_WHILE: return generate_while_stmt(node);
        case AST_FOR: return generate_for_stmt(node);
        case AST_CONTINUE: return generate_continue_stmt(node);
        case AST_BREAK: return generate_break_stmt(node);
        case AST_META_IF: return generate_meta_if_stmt(node);
        case AST_FUNCTION_CALL: return create_stmt(generate_expr(node));
        case AST_BLOCK: return create_stmt(generate_block(node));
        default: ASSERT_UNREACHABLE;
    }
}

sir::Stmt SIRGenerator::generate_var_stmt(ASTNode *node) {
    return create_stmt(sir::VarStmt{
        .ast_node = node,
        .name = generate_ident(node->get_child(VAR_NAME)),
        .type = generate_expr(node->get_child(VAR_TYPE)),
        .value = node->has_child(VAR_VALUE) ? generate_expr(node->get_child(VAR_VALUE)) : nullptr,
    });
}

sir::Stmt SIRGenerator::generate_typeless_var_stmt(ASTNode *node) {
    return create_stmt(sir::VarStmt{
        .ast_node = node,
        .name = generate_ident(node->get_child(TYPE_INFERRED_VAR_NAME)),
        .type = nullptr,
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

sir::MetaIfStmt *SIRGenerator::generate_meta_if_stmt(ASTNode *node) {
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
                .block = generate_meta_block(child->get_child(IF_BLOCK)),
            });
        } else if (child->get_type() == AST_META_ELSE) {
            sir_meta_if_stmt.else_branch = {
                .ast_node = child,
                .block = generate_meta_block(child->get_child()),
            };
        } else {
            ASSERT_UNREACHABLE;
        }
    }

    return create_stmt(sir_meta_if_stmt);
}

sir::Expr SIRGenerator::generate_expr(ASTNode *node) {
    switch (node->get_type()) {
        case AST_INT_LITERAL: return generate_int_literal(node);
        case AST_FLOAT_LITERAL: return generate_fp_literal(node);
        case AST_FALSE: return generate_bool_literal(node, false);
        case AST_TRUE: return generate_bool_literal(node, true);
        case AST_CHAR_LITERAL: return generate_char_literal(node);
        case AST_ARRAY_EXPR: return generate_array_literal(node);
        case AST_STRING_LITERAL: return generate_string_literal(node);
        case AST_STRUCT_INSTANTIATION: return generate_struct_literal(node);
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
        case AST_USIZE: return generate_primitive_type(node, sir::Primitive::I64); // FIXME: usize in sir?
        case AST_BOOL: return generate_primitive_type(node, sir::Primitive::BOOL);
        case AST_ADDR: return generate_primitive_type(node, sir::Primitive::ADDR);
        case AST_VOID: return generate_primitive_type(node, sir::Primitive::VOID);
        case AST_STATIC_ARRAY_TYPE: return generate_static_array_type(node);
        case AST_FUNCTION_DATA_TYPE: return generate_func_type(node);
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

sir::Expr SIRGenerator::generate_array_literal(ASTNode *node) {
    std::vector<sir::Expr> values;
    values.resize(node->get_children().size());

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
    sir::StructLiteral sir_struct_literal{
        .ast_node = node,
        .type = generate_expr(node->get_child(STRUCT_LITERAL_NAME)),
        .entries = {},
    };

    for (ASTNode *child : node->get_child(STRUCT_LITERAL_VALUES)->get_children()) {
        sir_struct_literal.entries.push_back({
            .ident = generate_ident(child->get_child(STRUCT_FIELD_VALUE_NAME)),
            .value = generate_expr(child->get_child(STRUCT_FIELD_VALUE_VALUE)),
            .field = nullptr,
        });
    }

    return create_expr(sir_struct_literal);
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
    return create_expr(sir::CallExpr{
        .ast_node = node,
        .type = nullptr,
        .callee = generate_expr(node->get_child(0)),
        .args = generate_arg_list(node->get_child(1)),
    });
}

std::vector<sir::Expr> SIRGenerator::generate_arg_list(ASTNode *node) {
    std::vector<sir::Expr> args;
    args.resize(node->get_children().size());

    for (unsigned i = 0; i < node->get_children().size(); i++) {
        args[i] = generate_expr(node->get_child(i));
    }

    return args;
}

sir::Expr SIRGenerator::generate_dot_expr(ASTNode *node) {
    ASTNode *rhs_node = node->get_child(1);
    sir::Ident rhs;

    if (rhs_node->get_type() == AST_IDENTIFIER) {
        rhs = generate_ident(rhs_node);
    } else if (rhs_node->get_type() == AST_INT_LITERAL) {
        rhs = {
            .ast_node = rhs_node,
            .value = rhs_node->as<IntLiteral>()->get_value().to_string(),
        };
    }

    return create_expr(sir::DotExpr{
        .ast_node = node,
        .lhs = generate_expr(node->get_child(0)),
        .rhs = rhs,
    });
}

sir::Expr SIRGenerator::generate_bracket_expr(ASTNode *node) {
    return create_expr(sir::BracketExpr{
        .ast_node = node,
        .lhs = generate_expr(node->get_child(0)),
        .rhs = generate_arg_list(node->get_child(1)),
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
    std::vector<sir::Expr> exprs;
    exprs.resize(node->get_children().size());

    for (unsigned i = 0; i < node->get_children().size(); i++) {
        exprs[i] = generate_expr(node->get_child(i));
    }

    return create_expr(sir::TupleExpr{
        .ast_node = node,
        .type = nullptr,
        .exprs = exprs,
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

sir::FuncType SIRGenerator::generate_func_type(ASTNode *params_node, ASTNode *return_node) {
    std::vector<sir::Param> sir_params;
    sir_params.resize(params_node->get_children().size());

    for (unsigned i = 0; i < params_node->get_children().size(); i++) {
        ASTNode *param_node = params_node->get_child(i);
        ASTNodeType ident_type = param_node->get_child(PARAM_NAME)->get_type();

        if (ident_type == AST_IDENTIFIER) {
            sir_params[i] = {
                .ast_node = param_node,
                .name = generate_ident(param_node->get_child(PARAM_NAME)),
                .type = generate_expr(param_node->get_child(PARAM_TYPE)),
            };
        } else if (ident_type == AST_SELF) {
            sir::Ident sir_ident{
                .ast_node = nullptr,
                .value = "self",
            };

            sir_params[i] = {
                .ast_node = param_node,
                .name = sir_ident,
                .type = nullptr,
            };
        } else {
            ASSERT_UNREACHABLE;
        }
    }

    return {
        .params = sir_params,
        .return_type = generate_expr(return_node),
    };
}

std::vector<sir::GenericParam> SIRGenerator::generate_generic_param_list(ASTNode *node) {
    std::vector<sir::GenericParam> sir_generic_params;
    sir_generic_params.resize(node->get_children().size());

    for (unsigned i = 0; i < node->get_children().size(); i++) {
        ASTNode *child = node->get_child(i);

        sir_generic_params[i] = sir::GenericParam{
            .ast_node = child,
            .ident = generate_ident(child->get_child(GENERIC_PARAM_NAME)),
        };
    }

    return sir_generic_params;
}

sir::Attributes *SIRGenerator::generate_attrs(const AttributeList &ast_attrs) {
    sir::Attributes sir_attrs;

    for (const Attribute &ast_attr : ast_attrs.get_attributes()) {
        if (ast_attr.get_name() == "exposed") sir_attrs.exposed = true;
        else if (ast_attr.get_name() == "dllexport") sir_attrs.dllexport = true;
        else if (ast_attr.get_name() == "link_name") sir_attrs.link_name = ast_attr.get_value();
        else ASSERT_UNREACHABLE;
    }

    return create_attrs(sir_attrs);
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
        .ast_node = node,
        .value = node->get_value(),
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
