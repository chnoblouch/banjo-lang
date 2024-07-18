#include "sir_printer.hpp"

#include "banjo/sir/sir.hpp"
#include "banjo/utils/macros.hpp"

#define BEGIN_OBJECT(name)                                                                                             \
    stream << name << "\n";                                                                                            \
    indent++;

#define PRINT_FIELD_NAME(name) stream << get_indent() << name << ": ";

#define PRINT_FIELD(name, value)                                                                                       \
    PRINT_FIELD_NAME(name);                                                                                            \
    stream << (value) << "\n";

#define PRINT_EXPR_FIELD(name, value)                                                                                  \
    PRINT_FIELD_NAME(name);                                                                                            \
    print_expr(value);

#define PRINT_BLOCK_FIELD(name, value)                                                                                 \
    PRINT_FIELD_NAME(name);                                                                                            \
    print_block(value);

#define BEGIN_LIST_FIELD(name)                                                                                         \
    PRINT_FIELD_NAME(name);                                                                                            \
    stream << "\n";                                                                                                    \
    indent++;

#define INDENT_LIST_ELEMENT() stream << get_indent();

#define END_LIST_FIELD() indent--;

#define END_OBJECT() indent--;

namespace banjo {

namespace lang {

namespace sir {

Printer::Printer(std::ostream &stream) : stream(stream) {}

void Printer::print(const Unit &unit) {
    BEGIN_OBJECT("Module");
    PRINT_FIELD_NAME("block");
    print_decl_block(unit.block);
    END_OBJECT();
}

void Printer::print_decl_block(const DeclBlock &decl_block) {
    stream << "\n";
    indent++;

    for (const Decl &decl : decl_block.decls) {
        stream << get_indent();
        print_decl(decl);
    }

    indent--;
}

void Printer::print_decl(const Decl &decl) {
    if (auto func_def = decl.match<FuncDef>()) print_func_def(*func_def);
    else if (auto native_func_decl = decl.match<NativeFuncDecl>()) print_native_func_decl(*native_func_decl);
    else if (auto struct_def = decl.match<StructDef>()) print_struct_def(*struct_def);
    else if (auto struct_field = decl.match<StructField>()) print_struct_field(*struct_field);
    else if (auto var_decl = decl.match<VarDecl>()) print_var_decl(*var_decl);
    else ASSERT_UNREACHABLE;
}

void Printer::print_func_def(const FuncDef &func_def) {
    BEGIN_OBJECT("FuncDef");
    PRINT_FIELD("ident", func_def.ident.value);
    PRINT_FIELD_NAME("type");
    print_func_type(func_def.type);
    PRINT_FIELD_NAME("block");
    print_block(func_def.block);
    END_OBJECT();
}

void Printer::print_native_func_decl(const NativeFuncDecl &native_func_decl) {
    BEGIN_OBJECT("NativeFuncDecl");
    PRINT_FIELD("ident", native_func_decl.ident.value);
    PRINT_FIELD_NAME("type");
    print_func_type(native_func_decl.type);
    END_OBJECT();
}

void Printer::print_struct_def(const StructDef &struct_def) {
    BEGIN_OBJECT("StructDef");
    PRINT_FIELD("ident", struct_def.ident.value);
    PRINT_FIELD_NAME("block");
    print_decl_block(struct_def.block);
    END_OBJECT();
}

void Printer::print_struct_field(const StructField &struct_field) {
    BEGIN_OBJECT("StructField");
    PRINT_FIELD("ident", struct_field.ident.value);
    PRINT_EXPR_FIELD("type", struct_field.type);
    END_OBJECT();
}

void Printer::print_var_decl(const VarDecl &var_decl) {
    BEGIN_OBJECT("VarDecl");
    PRINT_FIELD("ident", var_decl.ident.value);
    PRINT_EXPR_FIELD("type", var_decl.type);
    END_OBJECT();
}

void Printer::print_block(const Block &block) {
    stream << "\n";
    indent++;

    for (const Stmt &stmt : block.stmts) {
        stream << get_indent();
        print_stmt(stmt);
    }

    indent--;
}

void Printer::print_stmt(const Stmt &stmt) {
    if (auto var_stmt = stmt.match<VarStmt>()) print_var_stmt(*var_stmt);
    else if (auto assign_stmt = stmt.match<AssignStmt>()) print_assign_stmt(*assign_stmt);
    else if (auto return_stmt = stmt.match<ReturnStmt>()) print_return_stmt(*return_stmt);
    else if (auto if_stmt = stmt.match<IfStmt>()) print_if_stmt(*if_stmt);
    else if (auto expr = stmt.match<Expr>()) print_expr_stmt(*expr);
    else ASSERT_UNREACHABLE;
}

void Printer::print_var_stmt(const VarStmt &var_stmt) {
    BEGIN_OBJECT("VarStmt");
    PRINT_EXPR_FIELD("type", var_stmt.type);
    PRINT_EXPR_FIELD("value", var_stmt.value);
    END_OBJECT();
}

void Printer::print_assign_stmt(const AssignStmt &assign_stmt) {
    BEGIN_OBJECT("AssignStmt");
    PRINT_EXPR_FIELD("lhs", assign_stmt.lhs);
    PRINT_EXPR_FIELD("rhs", assign_stmt.rhs);
    END_OBJECT();
}

void Printer::print_return_stmt(const ReturnStmt &return_stmt) {
    BEGIN_OBJECT("ReturnStmt");
    PRINT_EXPR_FIELD("value", return_stmt.value);
    END_OBJECT();
}

void Printer::print_if_stmt(const IfStmt &if_stmt) {
    BEGIN_OBJECT("IfStmt");
    BEGIN_LIST_FIELD("cond_branches");

    for (const IfCondBranch &cond_branch : if_stmt.cond_branches) {
        INDENT_LIST_ELEMENT();
        BEGIN_OBJECT("IfCondBranch");
        PRINT_EXPR_FIELD("condition", cond_branch.condition);
        PRINT_BLOCK_FIELD("block", cond_branch.block);
        END_OBJECT();
    }

    END_LIST_FIELD();
    PRINT_FIELD_NAME("else_branch");

    if (if_stmt.else_branch) {
        BEGIN_OBJECT("IfElseBranch");
        PRINT_BLOCK_FIELD("block", if_stmt.else_branch->block);
        END_OBJECT();
    } else {
        stream << "none";
    }

    END_OBJECT();
}

void Printer::print_expr_stmt(const Expr &expr) {
    BEGIN_OBJECT("ExprStmt");
    PRINT_EXPR_FIELD("value", expr);
    END_OBJECT();
}

void Printer::print_expr(const Expr &expr) {
    if (!expr) stream << "none\n";
    else if (auto int_literal = expr.match<IntLiteral>()) print_int_literal(*int_literal);
    else if (auto fp_literal = expr.match<FPLiteral>()) print_fp_literal(*fp_literal);
    else if (auto bool_literal = expr.match<BoolLiteral>()) print_bool_literal(*bool_literal);
    else if (auto char_literal = expr.match<CharLiteral>()) print_char_literal(*char_literal);
    else if (auto string_literal = expr.match<StringLiteral>()) print_string_literal(*string_literal);
    else if (auto struct_literal = expr.match<StructLiteral>()) print_struct_literal(*struct_literal);
    else if (auto ident_expr = expr.match<IdentExpr>()) print_ident_expr(*ident_expr);
    else if (auto binary_expr = expr.match<BinaryExpr>()) print_binary_expr(*binary_expr);
    else if (auto unary_expr = expr.match<UnaryExpr>()) print_unary_expr(*unary_expr);
    else if (auto call_expr = expr.match<CallExpr>()) print_call_expr(*call_expr);
    else if (auto cast_expr = expr.match<CastExpr>()) print_cast_expr(*cast_expr);
    else if (auto dot_expr = expr.match<DotExpr>()) print_dot_expr(*dot_expr);
    else if (auto primitive_type = expr.match<PrimitiveType>()) print_primitive_type(*primitive_type);
    else if (auto pointer_type = expr.match<PointerType>()) print_pointer_type(*pointer_type);
    else if (auto func_type = expr.match<FuncType>()) print_func_type(*func_type);
    else if (auto star_expr = expr.match<StarExpr>()) print_star_expr(*star_expr);
    else ASSERT_UNREACHABLE;
}

void Printer::print_int_literal(const IntLiteral &int_literal) {
    BEGIN_OBJECT("IntLiteral");
    PRINT_EXPR_FIELD("type", int_literal.type);
    PRINT_FIELD("value", int_literal.value);
    END_OBJECT();
}

void Printer::print_fp_literal(const FPLiteral &fp_literal) {
    BEGIN_OBJECT("FPLiteral");
    PRINT_EXPR_FIELD("type", fp_literal.type);
    PRINT_FIELD("value", fp_literal.value);
    END_OBJECT();
}

void Printer::print_bool_literal(const BoolLiteral &bool_literal) {
    BEGIN_OBJECT("BoolLiteral");
    PRINT_EXPR_FIELD("type", bool_literal.type);
    PRINT_FIELD("value", bool_literal.value ? "true" : "false");
    END_OBJECT();
}

void Printer::print_char_literal(const CharLiteral &char_literal) {
    BEGIN_OBJECT("CharLiteral");
    PRINT_EXPR_FIELD("type", char_literal.type);
    PRINT_FIELD("value", char_literal.value);
    END_OBJECT();
}

void Printer::print_string_literal(const StringLiteral &string_literal) {
    BEGIN_OBJECT("StringLiteral");
    PRINT_EXPR_FIELD("type", string_literal.type);
    PRINT_FIELD("value", string_literal.value);
    END_OBJECT();
}

void Printer::print_struct_literal(const StructLiteral &struct_literal) {
    BEGIN_OBJECT("StructLiteral");
    PRINT_EXPR_FIELD("type", struct_literal.type);
    BEGIN_LIST_FIELD("entries");

    for (const sir::StructLiteralEntry &entry : struct_literal.entries) {
        INDENT_LIST_ELEMENT();
        BEGIN_OBJECT("StructLiteralEntry");
        PRINT_FIELD("field", entry.ident.value);
        PRINT_EXPR_FIELD("value", entry.value);
        END_OBJECT();
    }

    END_LIST_FIELD();
    END_OBJECT();
    END_OBJECT();
}

void Printer::print_ident_expr(const IdentExpr &ident_expr) {
    BEGIN_OBJECT("IdentExpr");
    PRINT_EXPR_FIELD("type", ident_expr.type);
    PRINT_FIELD("value", "\"" + ident_expr.value + "\"");
    END_OBJECT();
}

void Printer::print_binary_expr(const BinaryExpr &binary_expr) {
    BEGIN_OBJECT("BinaryExpr");

    switch (binary_expr.op) {
        case BinaryOp::ADD: PRINT_FIELD("op", "add"); break;
        case BinaryOp::SUB: PRINT_FIELD("op", "sub"); break;
        case BinaryOp::MUL: PRINT_FIELD("op", "mul"); break;
        case BinaryOp::DIV: PRINT_FIELD("op", "div"); break;
        case BinaryOp::MOD: PRINT_FIELD("op", "mod"); break;
        case BinaryOp::BIT_AND: PRINT_FIELD("op", "bitand"); break;
        case BinaryOp::BIT_OR: PRINT_FIELD("op", "bitor"); break;
        case BinaryOp::BIT_XOR: PRINT_FIELD("op", "bitxor"); break;
        case BinaryOp::SHL: PRINT_FIELD("op", "shl"); break;
        case BinaryOp::SHR: PRINT_FIELD("op", "shr"); break;
        case BinaryOp::EQ: PRINT_FIELD("op", "eq"); break;
        case BinaryOp::NE: PRINT_FIELD("op", "ne"); break;
        case BinaryOp::GT: PRINT_FIELD("op", "gt"); break;
        case BinaryOp::LT: PRINT_FIELD("op", "lt"); break;
        case BinaryOp::GE: PRINT_FIELD("op", "ge"); break;
        case BinaryOp::LE: PRINT_FIELD("op", "le"); break;
        case BinaryOp::AND: PRINT_FIELD("op", "and"); break;
        case BinaryOp::OR: PRINT_FIELD("op", "or"); break;
    }

    PRINT_EXPR_FIELD("type", binary_expr.type);
    PRINT_EXPR_FIELD("lhs", binary_expr.lhs);
    PRINT_EXPR_FIELD("rhs", binary_expr.rhs);
    END_OBJECT();
}

void Printer::print_unary_expr(const UnaryExpr &unary_expr) {
    BEGIN_OBJECT("UnaryExpr");

    switch (unary_expr.op) {
        case UnaryOp::NEG: PRINT_FIELD("op", "neg"); break;
        case UnaryOp::REF: PRINT_FIELD("op", "ref"); break;
        case UnaryOp::DEREF: PRINT_FIELD("op", "deref"); break;
        case UnaryOp::NOT: PRINT_FIELD("op", "not"); break;
    }

    PRINT_EXPR_FIELD("type", unary_expr.type);
    PRINT_EXPR_FIELD("value", unary_expr.value);
    END_OBJECT();
}

void Printer::print_cast_expr(const CastExpr &cast_expr) {
    BEGIN_OBJECT("CastExpr");
    PRINT_EXPR_FIELD("type", cast_expr.type);
    PRINT_EXPR_FIELD("value", cast_expr.value);
    END_OBJECT();
}

void Printer::print_call_expr(const CallExpr &call_expr) {
    BEGIN_OBJECT("CallExpr");
    PRINT_EXPR_FIELD("type", call_expr.type);
    PRINT_EXPR_FIELD("callee", call_expr.callee);
    BEGIN_LIST_FIELD("args");

    for (const sir::Expr &arg : call_expr.args) {
        INDENT_LIST_ELEMENT();
        print_expr(arg);
    }

    END_LIST_FIELD();
    END_OBJECT();
}

void Printer::print_dot_expr(const DotExpr &dot_expr) {
    BEGIN_OBJECT("DotExpr");
    PRINT_EXPR_FIELD("type", dot_expr.type);
    PRINT_EXPR_FIELD("lhs", dot_expr.lhs);
    PRINT_FIELD("rhs", dot_expr.rhs.value);
    END_OBJECT();
}

void Printer::print_primitive_type(const PrimitiveType &primitive_type) {
    BEGIN_OBJECT("PrimitiveType");

    switch (primitive_type.primitive) {
        case Primitive::I8: PRINT_FIELD("primitive", "i8"); break;
        case Primitive::I16: PRINT_FIELD("primitive", "i16"); break;
        case Primitive::I32: PRINT_FIELD("primitive", "i32"); break;
        case Primitive::I64: PRINT_FIELD("primitive", "i64"); break;
        case Primitive::U8: PRINT_FIELD("primitive", "u8"); break;
        case Primitive::U16: PRINT_FIELD("primitive", "u16"); break;
        case Primitive::U32: PRINT_FIELD("primitive", "u32"); break;
        case Primitive::U64: PRINT_FIELD("primitive", "u64"); break;
        case Primitive::F32: PRINT_FIELD("primitive", "f32"); break;
        case Primitive::F64: PRINT_FIELD("primitive", "f64"); break;
        case Primitive::BOOL: PRINT_FIELD("primitive", "bool"); break;
        case Primitive::ADDR: PRINT_FIELD("primitive", "addr"); break;
        case Primitive::VOID: PRINT_FIELD("primitive", "void"); break;
    }

    END_OBJECT();
}

void Printer::print_pointer_type(const PointerType &pointer_type) {
    BEGIN_OBJECT("PointerType");
    PRINT_EXPR_FIELD("base_type", pointer_type.base_type);
    END_OBJECT();
}

void Printer::print_func_type(const FuncType &func_type) {
    BEGIN_OBJECT("FuncType");
    BEGIN_LIST_FIELD("params");

    for (const Param &param : func_type.params) {
        INDENT_LIST_ELEMENT();
        BEGIN_OBJECT("Param");
        PRINT_FIELD("name", param.name.value);
        PRINT_EXPR_FIELD("type", param.type);
        END_OBJECT();
    }

    END_LIST_FIELD();
    PRINT_EXPR_FIELD("return_type", func_type.return_type);
    END_OBJECT();
}

void Printer::print_star_expr(const StarExpr &star_expr) {
    BEGIN_OBJECT("StarExpr");
    PRINT_EXPR_FIELD("type", star_expr.type);
    PRINT_EXPR_FIELD("value", star_expr.value);
    END_OBJECT();
}

std::string Printer::get_indent() {
    return std::string(2 * indent, ' ');
}

} // namespace sir

} // namespace lang

} // namespace banjo
