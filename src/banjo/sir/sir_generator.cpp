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

sir::Unit SIRGenerator::generate(ASTModule *mod) {
    sir_unit.block = generate_decl_block(mod->get_block());
    return std::move(sir_unit);
}

sir::DeclBlock SIRGenerator::generate_decl_block(ASTNode *node) {
    sir::SymbolTable *symbol_table = sir_unit.create_symbol_table({
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
        case AST_NATIVE_FUNCTION_DECLARATION: return generate_native_func(node);
        case AST_STRUCT_DEFINITION: return generate_struct(node);
        case AST_VAR: return generate_var_decl(node);
        default: ASSERT_UNREACHABLE;
    }
}

sir::Decl SIRGenerator::generate_func(ASTNode *node) {
    return sir_unit.create_decl(sir::FuncDef{
        .ast_node = node,
        .ident = generate_ident(node->get_child(FUNC_NAME)),
        .type = generate_func_type(node->get_child(FUNC_PARAMS), node->get_child(FUNC_TYPE)),
        .block = generate_block(node->get_child(FUNC_BLOCK)),
    });
}

sir::Decl SIRGenerator::generate_native_func(ASTNode *node) {
    return sir_unit.create_decl(sir::NativeFuncDecl{
        .ast_node = node,
        .ident = generate_ident(node->get_child(FUNC_NAME)),
        .type = generate_func_type(node->get_child(FUNC_PARAMS), node->get_child(FUNC_TYPE)),
    });
}

sir::Decl SIRGenerator::generate_struct(ASTNode *node) {
    return sir_unit.create_decl(sir::StructDef{
        .ast_node = node,
        .ident = generate_ident(node->get_child(STRUCT_NAME)),
        .block = generate_decl_block(node->get_child(STRUCT_BLOCK)),
        .fields = {},
    });
}

sir::Decl SIRGenerator::generate_var_decl(ASTNode *node) {
    return sir_unit.create_decl(sir::VarDecl{
        .ast_node = node,
        .ident = generate_ident(node->get_child(VAR_NAME)),
        .type = generate_expr(node->get_child(VAR_TYPE)),
    });
}

sir::Ident SIRGenerator::generate_ident(ASTNode *node) {
    return {
        .ast_node = node,
        .value = node->get_value(),
    };
}

sir::FuncType SIRGenerator::generate_func_type(ASTNode *params_node, ASTNode *return_node) {
    std::vector<sir::Param> params;
    params.resize(params_node->get_children().size());

    for (unsigned i = 0; i < params_node->get_children().size(); i++) {
        ASTNode *param_node = params_node->get_child(i);

        params[i] = {
            .node = param_node,
            .name = generate_ident(param_node->get_child(PARAM_NAME)),
            .type = generate_expr(param_node->get_child(PARAM_TYPE)),
        };
    }

    return {
        .params = params,
        .return_type = generate_expr(return_node),
    };
}

sir::Block SIRGenerator::generate_block(ASTNode *node) {
    sir::SymbolTable *symbol_table = sir_unit.create_symbol_table({
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

sir::Stmt SIRGenerator::generate_stmt(ASTNode *node) {
    switch (node->get_type()) {
        case AST_VAR: return generate_var_stmt(node);
        case AST_ASSIGNMENT: return generate_assign_stmt(node);
        case AST_IMPLICIT_TYPE_VAR: return generate_typeless_var_stmt(node);
        case AST_FUNCTION_RETURN: return generate_return_stmt(node);
        case AST_IF_CHAIN: return generate_if_stmt(node);
        case AST_FUNCTION_CALL: return sir_unit.create_stmt(generate_expr(node));
        default: ASSERT_UNREACHABLE;
    }
}

sir::Stmt SIRGenerator::generate_var_stmt(ASTNode *node) {
    return sir_unit.create_stmt(sir::VarStmt{
        .ast_node = node,
        .name = generate_ident(node->get_child(VAR_NAME)),
        .type = generate_expr(node->get_child(VAR_TYPE)),
        .value = node->has_child(VAR_VALUE) ? generate_expr(node->get_child(VAR_VALUE)) : nullptr,
    });
}

sir::Stmt SIRGenerator::generate_typeless_var_stmt(ASTNode *node) {
    return sir_unit.create_stmt(sir::VarStmt{
        .ast_node = node,
        .name = generate_ident(node->get_child(TYPE_INFERRED_VAR_NAME)),
        .type = nullptr,
        .value = generate_expr(node->get_child(TYPE_INFERRED_VAR_VALUE)),
    });
}

sir::Stmt SIRGenerator::generate_assign_stmt(ASTNode *node) {
    return sir_unit.create_stmt(sir::AssignStmt{
        .ast_node = node,
        .lhs = generate_expr(node->get_child(ASSIGN_LOCATION)),
        .rhs = generate_expr(node->get_child(ASSIGN_VALUE)),
    });
}

sir::Stmt SIRGenerator::generate_return_stmt(ASTNode *node) {
    return sir_unit.create_stmt(sir::ReturnStmt{
        .ast_node = node,
        .value = generate_expr(node->get_child()),
    });
}

sir::Stmt SIRGenerator::generate_if_stmt(ASTNode *node) {
    sir::IfStmt sir_stmt{
        .ast_node = node,
        .cond_branches = {},
        .else_branch = {},
    };

    for (ASTNode *child : node->get_children()) {
        if (child->get_type() == AST_IF || child->get_type() == AST_ELSE_IF) {
            sir_stmt.cond_branches.push_back({
                .ast_node = child,
                .condition = generate_expr(child->get_child(IF_CONDITION)),
                .block = generate_block(child->get_child(IF_BLOCK)),
            });
        } else if (child->get_type() == AST_ELSE) {
            sir_stmt.else_branch = {
                .ast_node = child,
                .block = generate_block(child->get_child()),
            };
        } else {
            ASSERT_UNREACHABLE;
        }
    }

    return sir_unit.create_stmt(sir_stmt);
}

sir::Expr SIRGenerator::generate_expr(ASTNode *node) {
    switch (node->get_type()) {
        case AST_INT_LITERAL: return generate_int_literal(node);
        case AST_FLOAT_LITERAL: return generate_fp_literal(node);
        case AST_FALSE: return generate_bool_literal(node, false);
        case AST_TRUE: return generate_bool_literal(node, true);
        case AST_CHAR_LITERAL: return generate_char_literal(node);
        case AST_STRING_LITERAL: return generate_string_literal(node);
        case AST_STRUCT_INSTANTIATION: return generate_struct_literal(node);
        case AST_IDENTIFIER: return generate_ident_expr(node);
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
        default: ASSERT_UNREACHABLE;
    }
}

sir::Expr SIRGenerator::generate_int_literal(ASTNode *node) {
    return sir_unit.create_expr(sir::IntLiteral{
        .ast_node = node,
        .type = nullptr,
        .value = node->as<IntLiteral>()->get_value(),
    });
}

sir::Expr SIRGenerator::generate_fp_literal(ASTNode *node) {
    return sir_unit.create_expr(sir::FPLiteral{
        .ast_node = node,
        .type = nullptr,
        .value = std::stod(node->get_value()),
    });
}

sir::Expr SIRGenerator::generate_bool_literal(ASTNode *node, bool value) {
    return sir_unit.create_expr(sir::BoolLiteral{
        .ast_node = node,
        .type = nullptr,
        .value = value,
    });
}

sir::Expr SIRGenerator::generate_char_literal(ASTNode *node) {
    unsigned index = 0;
    char value = decode_char(node->get_value(), index);

    return sir_unit.create_expr(sir::CharLiteral{
        .ast_node = node,
        .type = nullptr,
        .value = value,
    });
}

sir::Expr SIRGenerator::generate_string_literal(ASTNode *node) {
    unsigned index = 0;
    std::string value = "";

    while (index < node->get_value().length()) {
        value += decode_char(node->get_value(), index);
    }

    return sir_unit.create_expr(sir::StringLiteral{
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

    return sir_unit.create_expr(sir_struct_literal);
}

sir::Expr SIRGenerator::generate_ident_expr(ASTNode *node) {
    return sir_unit.create_expr(sir::IdentExpr{
        .ast_node = node,
        .type = nullptr,
        .value = node->get_value(),
        .symbol = nullptr,
    });
}

sir::Expr SIRGenerator::generate_binary_expr(ASTNode *node, sir::BinaryOp op) {
    return sir_unit.create_expr(sir::BinaryExpr{
        .ast_node = node,
        .type = nullptr,
        .op = op,
        .lhs = generate_expr(node->get_child(0)),
        .rhs = generate_expr(node->get_child(1)),
    });
}

sir::Expr SIRGenerator::generate_unary_expr(ASTNode *node, sir::UnaryOp op) {
    return sir_unit.create_expr(sir::UnaryExpr{
        .ast_node = node,
        .type = nullptr,
        .op = op,
        .value = generate_expr(node->get_child()),
    });
}

sir::Expr SIRGenerator::generate_cast_expr(ASTNode *node) {
    return sir_unit.create_expr(sir::CastExpr{
        .ast_node = node,
        .type = generate_expr(node->get_child(1)),
        .value = generate_expr(node->get_child(0)),
    });
}

sir::Expr SIRGenerator::generate_call_expr(ASTNode *node) {
    return sir_unit.create_expr(sir::CallExpr{
        .ast_node = node,
        .type = nullptr,
        .callee = generate_expr(node->get_child(0)),
        .args = generate_arg_list(node->get_child(1)),
    });
}

std::vector<sir::Expr> SIRGenerator::generate_arg_list(ASTNode *node) {
    std::vector<sir::Expr> arg_list;
    arg_list.resize(node->get_children().size());

    for (unsigned i = 0; i < node->get_children().size(); i++) {
        arg_list[i] = generate_expr(node->get_child(i));
    }

    return arg_list;
}

sir::Expr SIRGenerator::generate_dot_expr(ASTNode *node) {
    return sir_unit.create_expr(sir::DotExpr{
        .ast_node = node,
        .type = nullptr,
        .lhs = generate_expr(node->get_child(0)),
        .rhs = generate_ident(node->get_child(1)),
        .symbol = nullptr,
    });
}

sir::Expr SIRGenerator::generate_star_expr(ASTNode *node) {
    return sir_unit.create_expr(sir::StarExpr{
        .ast_node = node,
        .type = nullptr,
        .value = generate_expr(node->get_child()),
    });
}

sir::Expr SIRGenerator::generate_primitive_type(ASTNode *node, sir::Primitive primitive) {
    return sir_unit.create_expr(sir::PrimitiveType{
        .ast_node = node,
        .primitive = primitive,
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

} // namespace lang

} // namespace banjo
