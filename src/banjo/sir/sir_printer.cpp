#include "sir_printer.hpp"

#include "banjo/sir/sir.hpp"
#include "banjo/sir/sir_visitor.hpp"
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

#define PRINT_EXPR_LIST_FIELD(name, value)                                                                             \
    BEGIN_LIST_FIELD(name);                                                                                            \
                                                                                                                       \
    for (const Expr &expr : value) {                                                                                   \
        INDENT_LIST_ELEMENT();                                                                                         \
        print_expr(expr);                                                                                              \
    }                                                                                                                  \
                                                                                                                       \
    END_LIST();

#define PRINT_BLOCK_FIELD(name, value)                                                                                 \
    PRINT_FIELD_NAME(name);                                                                                            \
    print_block(value);

#define PRINT_META_BLOCK_FIELD(name, value)                                                                            \
    PRINT_FIELD_NAME(name);                                                                                            \
    print_meta_block(value);

#define BEGIN_LIST_FIELD(name)                                                                                         \
    PRINT_FIELD_NAME(name);                                                                                            \
    BEGIN_LIST();

#define BEGIN_LIST()                                                                                                   \
    stream << "\n";                                                                                                    \
    indent++

#define INDENT_LIST_ELEMENT() stream << get_indent();

#define END_LIST() indent--;

#define END_OBJECT() indent--;

namespace banjo {

namespace lang {

namespace sir {

Printer::Printer(std::ostream &stream) : stream(stream) {}

void Printer::print(const Unit &unit) {
    BEGIN_OBJECT("Unit");
    BEGIN_LIST_FIELD("mods");

    for (const Module *mod : unit.mods) {
        if (mod->path != ModulePath({"main"})) {
            continue;
        }

        INDENT_LIST_ELEMENT();
        print_mod(*mod);
    }

    END_LIST();
}

void Printer::print_mod(const Module &mod) {
    BEGIN_OBJECT("Module");
    PRINT_FIELD_NAME("block");
    print_decl_block(mod.block);
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
    SIR_VISIT_DECL(
        decl,
        SIR_VISIT_IMPOSSIBLE,
        print_func_def(*inner),
        print_native_func_decl(*inner),
        print_const_def(*inner),
        print_struct_def(*inner),
        print_struct_field(*inner),
        print_var_decl(*inner),
        print_enum_def(*inner),
        print_enum_variant(*inner),
        print_use_decl(*inner),
        print_meta_if_stmt(*inner),
        print_expanded_meta_stmt(*inner)
    );
}

void Printer::print_func_def(const FuncDef &func_def) {
    BEGIN_OBJECT("FuncDef");
    PRINT_FIELD("ident", func_def.ident.value);
    PRINT_FIELD_NAME("type");
    print_func_type(func_def.type);
    PRINT_FIELD_NAME("block");
    print_block(func_def.block);

    if (func_def.attrs) {
        PRINT_FIELD_NAME("attrs");
        print_attrs(*func_def.attrs);
    }

    if (func_def.is_generic()) {
        PRINT_FIELD_NAME("generic_params")
        print_generic_params(func_def.generic_params);
        BEGIN_LIST_FIELD("specializations")

        for (const Specialization<FuncDef> &specialization : func_def.specializations) {
            INDENT_LIST_ELEMENT()
            print_func_def(*specialization.def);
        }

        END_LIST()
    }

    END_OBJECT();
}

void Printer::print_native_func_decl(const NativeFuncDecl &native_func_decl) {
    BEGIN_OBJECT("NativeFuncDecl");
    PRINT_FIELD("ident", native_func_decl.ident.value);
    PRINT_FIELD_NAME("type");
    print_func_type(native_func_decl.type);

    if (native_func_decl.attrs) {
        PRINT_FIELD_NAME("attrs");
        print_attrs(*native_func_decl.attrs);
    }

    END_OBJECT();
}

void Printer::print_const_def(const ConstDef &const_def) {
    BEGIN_OBJECT("ConstDef");
    PRINT_FIELD("ident", const_def.ident.value);
    PRINT_EXPR_FIELD("type", const_def.type);
    PRINT_EXPR_FIELD("value", const_def.value);
    END_OBJECT();
}

void Printer::print_struct_def(const StructDef &struct_def) {
    BEGIN_OBJECT("StructDef");
    PRINT_FIELD("ident", struct_def.ident.value);
    PRINT_FIELD_NAME("block");
    print_decl_block(struct_def.block);

    if (struct_def.is_generic()) {
        PRINT_FIELD_NAME("generic_params")
        print_generic_params(struct_def.generic_params);
        BEGIN_LIST_FIELD("specializations")

        for (const Specialization<StructDef> &specialization : struct_def.specializations) {
            INDENT_LIST_ELEMENT()
            print_struct_def(*specialization.def);
        }

        END_LIST()
    }

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

void Printer::print_enum_def(const EnumDef &enum_def) {
    BEGIN_OBJECT("EnumDef");
    PRINT_FIELD("ident", enum_def.ident.value);
    PRINT_FIELD_NAME("block");
    print_decl_block(enum_def.block);
    END_OBJECT();
}

void Printer::print_enum_variant(const EnumVariant &enum_variant) {
    BEGIN_OBJECT("EnumVariant");
    PRINT_FIELD("ident", enum_variant.ident.value);
    PRINT_EXPR_FIELD("value", enum_variant.value);
    END_OBJECT();
}

void Printer::print_use_decl(const UseDecl &use_decl) {
    BEGIN_OBJECT("UseDecl");
    PRINT_FIELD_NAME("root_item");
    print_use_item(use_decl.root_item);
    END_OBJECT();
}

void Printer::print_use_item(const UseItem &use_item) {
    if (auto use_ident = use_item.match<UseIdent>()) print_use_ident(*use_ident);
    else if (auto use_rebind = use_item.match<UseRebind>()) print_use_rebind(*use_rebind);
    else if (auto use_dot_expr = use_item.match<UseDotExpr>()) print_use_dot_expr(*use_dot_expr);
    else if (auto use_list = use_item.match<UseList>()) print_use_list(*use_list);
    else ASSERT_UNREACHABLE;
}

void Printer::print_use_ident(const UseIdent &use_ident) {
    BEGIN_OBJECT("UseIdent");
    PRINT_FIELD("value", use_ident.ident.value);
    END_OBJECT();
}

void Printer::print_use_rebind(const UseRebind &use_rebind) {
    BEGIN_OBJECT("UseRebind");
    PRINT_FIELD("target", use_rebind.target_ident.value);
    PRINT_FIELD("local", use_rebind.local_ident.value);
    END_OBJECT();
}

void Printer::print_use_dot_expr(const UseDotExpr &use_dot_expr) {
    BEGIN_OBJECT("UseDotExpr");
    PRINT_FIELD_NAME("lhs");
    print_use_item(use_dot_expr.lhs);
    PRINT_FIELD_NAME("rhs");
    print_use_item(use_dot_expr.rhs);
    END_OBJECT();
}

void Printer::print_use_list(const UseList &use_list) {
    BEGIN_OBJECT("UseList");
    BEGIN_LIST_FIELD("items");

    for (const UseItem &item : use_list.items) {
        INDENT_LIST_ELEMENT();
        print_use_item(item);
    }

    END_LIST();
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
    SIR_VISIT_STMT(
        stmt,
        SIR_VISIT_IMPOSSIBLE,
        print_var_stmt(*inner),
        print_assign_stmt(*inner),
        print_comp_assign_stmt(*inner),
        print_return_stmt(*inner),
        print_if_stmt(*inner),
        print_while_stmt(*inner),
        print_for_stmt(*inner),
        print_loop_stmt(*inner),
        print_continue_stmt(*inner),
        print_break_stmt(*inner),
        print_meta_if_stmt(*inner),
        print_expanded_meta_stmt(*inner),
        print_expr_stmt(*inner),
        print_block_stmt(*inner)
    );
}

void Printer::print_var_stmt(const VarStmt &var_stmt) {
    BEGIN_OBJECT("VarStmt");
    PRINT_FIELD("name", var_stmt.name.value);
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

void Printer::print_comp_assign_stmt(const CompAssignStmt &comp_assign_stmt) {
    BEGIN_OBJECT("CompAssignStmt");
    print_binary_op("op", comp_assign_stmt.op);
    PRINT_EXPR_FIELD("lhs", comp_assign_stmt.lhs);
    PRINT_EXPR_FIELD("rhs", comp_assign_stmt.rhs);
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

    END_LIST();
    PRINT_FIELD_NAME("else_branch");

    if (if_stmt.else_branch) {
        BEGIN_OBJECT("IfElseBranch");
        PRINT_BLOCK_FIELD("block", if_stmt.else_branch->block);
        END_OBJECT();
    } else {
        stream << "none\n";
    }

    END_OBJECT();
}

void Printer::print_while_stmt(const WhileStmt &while_stmt) {
    BEGIN_OBJECT("WhileStmt");
    PRINT_EXPR_FIELD("condition", while_stmt.condition);
    PRINT_BLOCK_FIELD("block", while_stmt.block);
    END_OBJECT();
}

void Printer::print_for_stmt(const ForStmt &for_stmt) {
    BEGIN_OBJECT("ForStmt");
    PRINT_FIELD("ident", for_stmt.ident.value);
    PRINT_EXPR_FIELD("range", for_stmt.range);
    PRINT_BLOCK_FIELD("block", for_stmt.block);
    END_OBJECT();
}

void Printer::print_loop_stmt(const LoopStmt &loop_stmt) {
    BEGIN_OBJECT("LoopStmt");
    PRINT_EXPR_FIELD("condition", loop_stmt.condition);
    PRINT_BLOCK_FIELD("block", loop_stmt.block);

    if (loop_stmt.latch) {
        PRINT_BLOCK_FIELD("latch", *loop_stmt.latch);
    } else {
        PRINT_FIELD("latch", "none");
    }

    END_OBJECT();
}

void Printer::print_continue_stmt(const ContinueStmt & /*continue_stmt*/) {
    BEGIN_OBJECT("ContinueStmt");
    END_OBJECT();
}

void Printer::print_break_stmt(const BreakStmt & /*break_stmt*/) {
    BEGIN_OBJECT("BreakStmt");
    END_OBJECT();
}

void Printer::print_meta_if_stmt(const MetaIfStmt &meta_if_stmt) {
    BEGIN_OBJECT("MetaIfStmt");
    BEGIN_LIST_FIELD("cond_branches");

    for (const MetaIfCondBranch &cond_branch : meta_if_stmt.cond_branches) {
        INDENT_LIST_ELEMENT();
        BEGIN_OBJECT("MetaIfCondBranch");
        PRINT_EXPR_FIELD("condition", cond_branch.condition);
        PRINT_META_BLOCK_FIELD("block", cond_branch.block);
        END_OBJECT();
    }

    END_LIST();
    PRINT_FIELD_NAME("else_branch");

    if (meta_if_stmt.else_branch) {
        BEGIN_OBJECT("MetaIfElseBranch");
        PRINT_META_BLOCK_FIELD("block", meta_if_stmt.else_branch->block);
        END_OBJECT();
    } else {
        stream << "none\n";
    }

    END_OBJECT();
}

void Printer::print_expanded_meta_stmt(const ExpandedMetaStmt & /*expanded_meta_stmt*/) {
    BEGIN_OBJECT("ExpandedMetaStmt");
    END_OBJECT();
}

void Printer::print_expr_stmt(const Expr &expr) {
    BEGIN_OBJECT("Expr");
    PRINT_EXPR_FIELD("value", expr);
    END_OBJECT();
}

void Printer::print_block_stmt(const Block &block) {
    BEGIN_OBJECT("Block");
    PRINT_BLOCK_FIELD("block", block);
    END_OBJECT();
}

void Printer::print_expr(const Expr &expr) {
    SIR_VISIT_EXPR(
        expr,
        stream << "none\n",
        print_int_literal(*inner),
        print_fp_literal(*inner),
        print_bool_literal(*inner),
        print_char_literal(*inner),
        print_null_literal(*inner),
        print_array_literal(*inner),
        print_string_literal(*inner),
        print_struct_literal(*inner),
        print_symbol_expr(*inner),
        print_binary_expr(*inner),
        print_unary_expr(*inner),
        print_cast_expr(*inner),
        print_index_expr(*inner),
        print_call_expr(*inner),
        print_field_expr(*inner),
        print_range_expr(*inner),
        print_tuple_expr(*inner),
        print_primitive_type(*inner),
        print_pointer_type(*inner),
        print_static_array_type(*inner),
        print_func_type(*inner),
        print_ident_expr(*inner),
        print_star_expr(*inner),
        print_bracket_expr(*inner),
        print_dot_expr(*inner)
    );
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

void Printer::print_null_literal(const NullLiteral &null_literal) {
    BEGIN_OBJECT("NullLiteral");
    PRINT_EXPR_FIELD("type", null_literal.type);
    END_OBJECT();
}

void Printer::print_array_literal(const ArrayLiteral &array_literal) {
    BEGIN_OBJECT("ArrayLiteral");
    PRINT_EXPR_FIELD("type", array_literal.type);
    PRINT_EXPR_LIST_FIELD("values", array_literal.values);
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

    for (const StructLiteralEntry &entry : struct_literal.entries) {
        INDENT_LIST_ELEMENT();
        BEGIN_OBJECT("StructLiteralEntry");
        PRINT_FIELD("field", entry.ident.value);
        PRINT_EXPR_FIELD("value", entry.value);
        END_OBJECT();
    }

    END_LIST();
    END_OBJECT();
    END_OBJECT();
}

void Printer::print_symbol_expr(const SymbolExpr &symbol_expr) {
    BEGIN_OBJECT("SymbolExpr");
    PRINT_EXPR_FIELD("type", symbol_expr.type);
    PRINT_FIELD("name", "\"" + symbol_expr.symbol.get_name() + "\"");
    END_OBJECT();
}

void Printer::print_binary_expr(const BinaryExpr &binary_expr) {
    BEGIN_OBJECT("BinaryExpr");
    PRINT_EXPR_FIELD("type", binary_expr.type);
    print_binary_op("op", binary_expr.op);
    PRINT_EXPR_FIELD("lhs", binary_expr.lhs);
    PRINT_EXPR_FIELD("rhs", binary_expr.rhs);
    END_OBJECT();
}

void Printer::print_unary_expr(const UnaryExpr &unary_expr) {
    BEGIN_OBJECT("UnaryExpr");

    switch (unary_expr.op) {
        case UnaryOp::NEG: PRINT_FIELD("op", "NEG"); break;
        case UnaryOp::REF: PRINT_FIELD("op", "REF"); break;
        case UnaryOp::DEREF: PRINT_FIELD("op", "DEREF"); break;
        case UnaryOp::NOT: PRINT_FIELD("op", "NOT"); break;
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

void Printer::print_index_expr(const IndexExpr &index_expr) {
    BEGIN_OBJECT("IndexExpr");
    PRINT_EXPR_FIELD("type", index_expr.type);
    PRINT_EXPR_FIELD("base", index_expr.base);
    PRINT_EXPR_FIELD("index", index_expr.index);
    END_OBJECT();
}

void Printer::print_call_expr(const CallExpr &call_expr) {
    BEGIN_OBJECT("CallExpr");
    PRINT_EXPR_FIELD("type", call_expr.type);
    PRINT_EXPR_FIELD("callee", call_expr.callee);
    PRINT_EXPR_LIST_FIELD("args", call_expr.args);
    END_OBJECT();
}

void Printer::print_field_expr(const FieldExpr &field_expr) {
    BEGIN_OBJECT("DotExpr");
    PRINT_EXPR_FIELD("type", field_expr.type);
    PRINT_EXPR_FIELD("base", field_expr.base);
    PRINT_FIELD("field", field_expr.field_index);
    END_OBJECT();
}

void Printer::print_range_expr(const RangeExpr &range_expr) {
    BEGIN_OBJECT("DotExpr");
    PRINT_EXPR_FIELD("lhs", range_expr.lhs);
    PRINT_EXPR_FIELD("rhs", range_expr.rhs);
    END_OBJECT();
}

void Printer::print_tuple_expr(const TupleExpr &tuple_expr) {
    BEGIN_OBJECT("TupleExpr");
    PRINT_EXPR_FIELD("type", tuple_expr.type);
    PRINT_EXPR_LIST_FIELD("exprs", tuple_expr.exprs);
    END_OBJECT();
}

void Printer::print_primitive_type(const PrimitiveType &primitive_type) {
    BEGIN_OBJECT("PrimitiveType");

    switch (primitive_type.primitive) {
        case Primitive::I8: PRINT_FIELD("primitive", "I8"); break;
        case Primitive::I16: PRINT_FIELD("primitive", "I16"); break;
        case Primitive::I32: PRINT_FIELD("primitive", "I32"); break;
        case Primitive::I64: PRINT_FIELD("primitive", "I64"); break;
        case Primitive::U8: PRINT_FIELD("primitive", "U8"); break;
        case Primitive::U16: PRINT_FIELD("primitive", "U16"); break;
        case Primitive::U32: PRINT_FIELD("primitive", "U32"); break;
        case Primitive::U64: PRINT_FIELD("primitive", "U64"); break;
        case Primitive::F32: PRINT_FIELD("primitive", "F32"); break;
        case Primitive::F64: PRINT_FIELD("primitive", "F64"); break;
        case Primitive::BOOL: PRINT_FIELD("primitive", "BOOL"); break;
        case Primitive::ADDR: PRINT_FIELD("primitive", "ADDR"); break;
        case Primitive::VOID: PRINT_FIELD("primitive", "VOID"); break;
    }

    END_OBJECT();
}

void Printer::print_pointer_type(const PointerType &pointer_type) {
    BEGIN_OBJECT("PointerType");
    PRINT_EXPR_FIELD("base_type", pointer_type.base_type);
    END_OBJECT();
}

void Printer::print_static_array_type(const StaticArrayType &static_array_type) {
    BEGIN_OBJECT("StaticArrayType");
    PRINT_EXPR_FIELD("base_type", static_array_type.base_type);
    PRINT_EXPR_FIELD("length", static_array_type.length);
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

    END_LIST();
    PRINT_EXPR_FIELD("return_type", func_type.return_type);
    END_OBJECT();
}

void Printer::print_ident_expr(const IdentExpr &ident_expr) {
    BEGIN_OBJECT("IdentExpr");
    PRINT_FIELD("value", "\"" + ident_expr.value + "\"");
    END_OBJECT();
}

void Printer::print_star_expr(const StarExpr &star_expr) {
    BEGIN_OBJECT("StarExpr");
    PRINT_EXPR_FIELD("value", star_expr.value);
    END_OBJECT();
}

void Printer::print_bracket_expr(const BracketExpr &bracket_expr) {
    BEGIN_OBJECT("BracketExpr");
    PRINT_EXPR_FIELD("lhs", bracket_expr.lhs);
    PRINT_EXPR_LIST_FIELD("rhs", bracket_expr.rhs);
    END_OBJECT();
}

void Printer::print_dot_expr(const DotExpr &dot_expr) {
    BEGIN_OBJECT("DotExpr");
    PRINT_EXPR_FIELD("lhs", dot_expr.lhs);
    PRINT_FIELD("rhs", dot_expr.rhs.value);
    END_OBJECT();
}

void Printer::print_generic_params(const std::vector<GenericParam> &generic_params) {
    BEGIN_LIST();

    for (const GenericParam &generic_param : generic_params) {
        INDENT_LIST_ELEMENT();
        BEGIN_OBJECT("GenericParam")
        PRINT_FIELD("ident", generic_param.ident.value)
        END_OBJECT()
    }

    END_LIST()
}

void Printer::print_attrs(const Attributes &attrs) {
    BEGIN_OBJECT("Attributes");
    PRINT_FIELD("exposed", attrs.exposed ? "true" : "false");
    PRINT_FIELD("dllexport", attrs.dllexport ? "true" : "false");
    PRINT_FIELD("link_name", attrs.link_name ? "\"" + *attrs.link_name + "\"" : "none");
    END_OBJECT();
}

void Printer::print_binary_op(const char *field_name, BinaryOp op) {
    switch (op) {
        case BinaryOp::ADD: PRINT_FIELD(field_name, "ADD"); break;
        case BinaryOp::SUB: PRINT_FIELD(field_name, "SUB"); break;
        case BinaryOp::MUL: PRINT_FIELD(field_name, "MUL"); break;
        case BinaryOp::DIV: PRINT_FIELD(field_name, "DIV"); break;
        case BinaryOp::MOD: PRINT_FIELD(field_name, "MOD"); break;
        case BinaryOp::BIT_AND: PRINT_FIELD(field_name, "BIT_AND"); break;
        case BinaryOp::BIT_OR: PRINT_FIELD(field_name, "BIT_OR"); break;
        case BinaryOp::BIT_XOR: PRINT_FIELD(field_name, "BIT_XOR"); break;
        case BinaryOp::SHL: PRINT_FIELD(field_name, "SHL"); break;
        case BinaryOp::SHR: PRINT_FIELD(field_name, "SHR"); break;
        case BinaryOp::EQ: PRINT_FIELD(field_name, "EQ"); break;
        case BinaryOp::NE: PRINT_FIELD(field_name, "NE"); break;
        case BinaryOp::GT: PRINT_FIELD(field_name, "GT"); break;
        case BinaryOp::LT: PRINT_FIELD(field_name, "LT"); break;
        case BinaryOp::GE: PRINT_FIELD(field_name, "GE"); break;
        case BinaryOp::LE: PRINT_FIELD(field_name, "LE"); break;
        case BinaryOp::AND: PRINT_FIELD(field_name, "AND"); break;
        case BinaryOp::OR: PRINT_FIELD(field_name, "OR"); break;
    }
}

void Printer::print_meta_block(const MetaBlock &meta_block) {
    stream << "\n";
    indent++;

    for (const Node &node : meta_block.nodes) {
        stream << get_indent();

        if (auto expr = node.match<sir::Expr>()) print_expr(*expr);
        else if (auto stmt = node.match<sir::Stmt>()) print_stmt(*stmt);
        else if (auto decl = node.match<sir::Decl>()) print_decl(*decl);
        else ASSERT_UNREACHABLE;
    }

    indent--;
}

std::string Printer::get_indent() {
    return std::string(2 * indent, ' ');
}

} // namespace sir

} // namespace lang

} // namespace banjo
