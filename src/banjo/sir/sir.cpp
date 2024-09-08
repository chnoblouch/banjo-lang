#include "sir.hpp"

#include "banjo/sir/sir_visitor.hpp"

#include <utility>

namespace banjo {

namespace lang {

namespace sir {

// This currently only supports comparing types.
bool Expr::operator==(const Expr &other) const {
    if (kind.index() != other.kind.index()) {
        return false;
    }

    SIR_VISIT_EXPR(
        *this,
        SIR_VISIT_IMPOSSIBLE,                                     // empty
        return inner->value == other.as<sir::IntLiteral>().value, // int_literal
        SIR_VISIT_IMPOSSIBLE,                                     // fp_literal
        SIR_VISIT_IMPOSSIBLE,                                     // bool_literal
        SIR_VISIT_IMPOSSIBLE,                                     // char_literal
        SIR_VISIT_IMPOSSIBLE,                                     // null_literal
        SIR_VISIT_IMPOSSIBLE,                                     // none_literal
        SIR_VISIT_IMPOSSIBLE,                                     // undefined_literal
        SIR_VISIT_IMPOSSIBLE,                                     // array_literal
        SIR_VISIT_IMPOSSIBLE,                                     // string_literal
        SIR_VISIT_IMPOSSIBLE,                                     // struct_literal
        SIR_VISIT_IMPOSSIBLE,                                     // closure_literal
        return inner->symbol == other.as<SymbolExpr>().symbol,    // symbol_expr
        SIR_VISIT_IMPOSSIBLE,                                     // binary_expr
        SIR_VISIT_IMPOSSIBLE,                                     // unary_expr
        SIR_VISIT_IMPOSSIBLE,                                     // cast_expr
        SIR_VISIT_IMPOSSIBLE,                                     // index_expr
        SIR_VISIT_IMPOSSIBLE,                                     // call_expr
        SIR_VISIT_IMPOSSIBLE,                                     // field_expr
        SIR_VISIT_IMPOSSIBLE,                                     // range_expr
        return *inner == other.as<TupleExpr>(),                   // tuple_expr
        return *inner == other.as<PrimitiveType>(),               // primitive_type
        return *inner == other.as<PointerType>(),                 // pointer_type
        return *inner == other.as<StaticArrayType>(),             // static_array_type
        return *inner == other.as<FuncType>(),                    // func_type
        return *inner == other.as<OptionalType>(),                // optional_type
        return *inner == other.as<ArrayType>(),                   // array_type
        return *inner == other.as<ClosureType>(),                 // closure_type
        SIR_VISIT_IMPOSSIBLE,                                     // ident_expr
        SIR_VISIT_IMPOSSIBLE,                                     // star_expr
        SIR_VISIT_IMPOSSIBLE,                                     // bracket_expr
        SIR_VISIT_IMPOSSIBLE,                                     // dot_expr
        SIR_VISIT_IMPOSSIBLE,                                     // meta_access
        SIR_VISIT_IMPOSSIBLE,                                     // meta_field_expr
        SIR_VISIT_IMPOSSIBLE                                      // meta_call_expr
    );
}

bool Expr::is_value() const {
    return get_type();
}

Expr Expr::get_type() const {
    SIR_VISIT_EXPR(
        *this,
        return nullptr,       // empty
        return inner->type,   // int_literal
        return inner->type,   // fp_literal
        return inner->type,   // bool_literal
        return inner->type,   // char_literal
        return inner->type,   // null_literal
        return inner->type,   // none_literal
        return inner->type,   // undefined_literal
        return inner->type,   // array_literal
        return inner->type,   // string_literal
        return inner->type,   // struct_literal
        return inner->type,   // closure_literal
        return inner->type,   // symbol_expr
        return inner->type,   // binary_expr
        return inner->type,   // unary_expr
        return inner->type,   // cast_expr
        return inner->type,   // index_expr
        return inner->type,   // call_expr
        return inner->type,   // field_expr
        return nullptr,       // range_expr
        return inner->type,   // tuple_expr
        return nullptr,       // primitive_type
        return nullptr,       // pointer_type
        return nullptr,       // static_array_type
        return nullptr,       // func_type
        return nullptr,       // optional_type
        return nullptr,       // array_type
        return nullptr,       // closure_type
        return nullptr,       // ident_expr
        return nullptr,       // star_expr
        return nullptr,       // bracket_expr
        return nullptr,       // dot_expr
        SIR_VISIT_IMPOSSIBLE, // meta_access
        SIR_VISIT_IMPOSSIBLE, // meta_field_expr
        SIR_VISIT_IMPOSSIBLE  // meta_call_expr
    );
}

bool Expr::is_type() const {
    if (auto symbol_expr = match<SymbolExpr>()) {
        const Symbol &symbol = symbol_expr->symbol;
        return symbol.is<StructDef>() || symbol.is<EnumDef>();
    } else if (auto tuple_expr = match<TupleExpr>()) {
        return tuple_expr->exprs.empty() || tuple_expr->exprs[0].is_type();
    } else {
        return is<PrimitiveType>() || is<PointerType>() || is<FuncType>();
    }
}

bool Expr::is_primitive_type(Primitive primitive) const {
    if (auto primitive_type = match<PrimitiveType>()) {
        return primitive_type->primitive == primitive;
    } else {
        return false;
    }
}

bool Expr::is_int_type() const {
    if (auto primitive_type = match<PrimitiveType>()) {
        switch (primitive_type->primitive) {
            case Primitive::I8:
            case Primitive::I16:
            case Primitive::I32:
            case Primitive::I64:
            case Primitive::U8:
            case Primitive::U16:
            case Primitive::U32:
            case Primitive::U64: return true;
            default: return false;
        }
    } else {
        return false;
    }
}

bool Expr::is_signed_type() const {
    if (auto primitive_type = match<PrimitiveType>()) {
        switch (primitive_type->primitive) {
            case Primitive::I8:
            case Primitive::I16:
            case Primitive::I32:
            case Primitive::I64: return true;
            default: return false;
        }
    } else {
        return false;
    }
}

bool Expr::is_unsigned_type() const {
    if (auto primitive_type = match<PrimitiveType>()) {
        switch (primitive_type->primitive) {
            case Primitive::U8:
            case Primitive::U16:
            case Primitive::U32:
            case Primitive::U64: return true;
            default: return false;
        }
    } else {
        return false;
    }
}

bool Expr::is_fp_type() const {
    if (auto primitive_type = match<PrimitiveType>()) {
        switch (primitive_type->primitive) {
            case Primitive::F32:
            case Primitive::F64: return true;
            default: return false;
        }
    } else {
        return false;
    }
}

bool Expr::is_addr_like_type() const {
    return is_primitive_type(sir::Primitive::ADDR) || is<sir::PointerType>() || is<sir::FuncType>();
}

bool Expr::is_u8_ptr() const {
    if (auto pointer_type = match<PointerType>()) {
        return pointer_type->base_type.is_primitive_type(sir::Primitive::U8);
    } else {
        return false;
    }
}

bool Expr::is_symbol(sir::Symbol symbol) const {
    if (auto symbol_expr = match<SymbolExpr>()) {
        return symbol_expr->symbol == symbol;
    } else {
        return false;
    }
}

ASTNode *Expr::get_ast_node() const {
    SIR_VISIT_EXPR(
        *this,
        SIR_VISIT_IMPOSSIBLE,
        return inner->ast_node,
        return inner->ast_node,
        return inner->ast_node,
        return inner->ast_node,
        return inner->ast_node,
        return inner->ast_node,
        return inner->ast_node,
        return inner->ast_node,
        return inner->ast_node,
        return inner->ast_node,
        return inner->ast_node,
        return inner->ast_node,
        return inner->ast_node,
        return inner->ast_node,
        return inner->ast_node,
        return inner->ast_node,
        return inner->ast_node,
        return inner->ast_node,
        return inner->ast_node,
        return inner->ast_node,
        return inner->ast_node,
        return inner->ast_node,
        return inner->ast_node,
        return inner->ast_node,
        return inner->ast_node,
        return inner->ast_node,
        return inner->ast_node,
        return inner->ast_node,
        return inner->ast_node,
        return inner->ast_node,
        return inner->ast_node,
        return inner->ast_node,
        return inner->ast_node,
        return inner->ast_node
    );
}

DeclBlock *Expr::get_decl_block() {
    if (auto symbol_expr = match<SymbolExpr>()) {
        return symbol_expr->symbol.get_decl_block();
    } else {
        return nullptr;
    }
}

bool Symbol::operator==(const Symbol &other) const {
    if (kind.index() != other.kind.index()) {
        return false;
    }

    SIR_VISIT_SYMBOL(
        *this,
        SIR_VISIT_IMPOSSIBLE,
        return inner == &other.as<Module>(),
        return inner == &other.as<FuncDef>(),
        return inner == &other.as<NativeFuncDecl>(),
        return inner == &other.as<ConstDef>(),
        return inner == &other.as<StructDef>(),
        return inner == &other.as<StructField>(),
        return inner == &other.as<VarDecl>(),
        return inner == &other.as<NativeVarDecl>(),
        return inner == &other.as<EnumDef>(),
        return inner == &other.as<EnumVariant>(),
        return inner == &other.as<TypeAlias>(),
        return inner == &other.as<UseIdent>(),
        return inner == &other.as<UseRebind>(),
        return inner == &other.as<VarStmt>(),
        return inner == &other.as<Param>(),
        return inner == &other.as<OverloadSet>()
    );
}

const Ident &Symbol::get_ident() const {
    SIR_VISIT_SYMBOL(
        *this,
        SIR_VISIT_IMPOSSIBLE, // empty
        SIR_VISIT_IMPOSSIBLE, // module
        return inner->ident,  // func_def
        return inner->ident,  // native_func_decl
        return inner->ident,  // const_def
        return inner->ident,  // struct_def
        return inner->ident,  // struct_field
        return inner->ident,  // var_decl
        return inner->ident,  // native_var_decl
        return inner->ident,  // enum_def
        return inner->ident,  // enum_variant
        return inner->ident,  // type_alias
        return inner->ident,  // use_ident
        SIR_VISIT_IMPOSSIBLE, // use_rebind
        return inner->name,   // var_stmt
        return inner->name,   // param
        SIR_VISIT_IMPOSSIBLE  // overload_set
    );
}

Ident &Symbol::get_ident() {
    return const_cast<Ident &>(std::as_const(*this).get_ident());
}

std::string Symbol::get_name() const {
    if (auto mod = match<Module>()) {
        return mod->path.to_string();
    } else {
        return get_ident().value;
    }
}

Expr Symbol::get_type() {
    SIR_VISIT_SYMBOL(
        *this,
        return nullptr,      // empty
        return nullptr,      // module
        return &inner->type, // func_def
        return &inner->type, // native_func_decl
        return inner->type,  // const_def
        return nullptr,      // struct_def
        return inner->type,  // struct_field
        return inner->type,  // var_decl
        return inner->type,  // native_var_decl
        return nullptr,      // enum_def
        return inner->type,  // enum_variant
        return nullptr,      // type_alias
        return nullptr,      // use_ident
        return nullptr,      // use_rebind
        return inner->type,  // var_stmt
        return inner->type,  // param
        return nullptr       // overload_set
    );
}

Symbol Symbol::resolve() {
    if (auto use_ident = match<UseIdent>()) return use_ident->symbol;
    else if (auto use_rebind = match<UseRebind>()) return use_rebind->symbol;
    else return *this;
}

SymbolTable *Symbol::get_symbol_table() {
    if (auto mod = match<Module>()) return mod->block.symbol_table;
    else if (auto struct_def = match<StructDef>()) return struct_def->block.symbol_table;
    else if (auto enum_def = match<EnumDef>()) return struct_def->block.symbol_table;
    else return nullptr;
}

DeclBlock *Symbol::get_decl_block() {
    if (auto mod = match<Module>()) return &mod->block;
    else if (auto struct_def = match<StructDef>()) return &struct_def->block;
    else if (auto enum_def = match<EnumDef>()) return &enum_def->block;
    else return nullptr;
}

Symbol SymbolTable::look_up(std::string_view name) {
    auto iter = symbols.find(name);
    if (iter == symbols.end()) {
        return parent ? parent->look_up(name) : nullptr;
    } else {
        return iter->second.resolve();
    }
}

bool PseudoType::is_struct_by_default() const {
    switch (kind) {
        case PseudoTypeKind::ARRAY_LITERAL:
        case PseudoTypeKind::STRING_LITERAL: return true;
        default: return false;
    }
}

StructField *StructDef::find_field(std::string_view name) const {
    for (StructField *field : fields) {
        if (field->ident.value == name) {
            return field;
        }
    }

    return nullptr;
}

} // namespace sir

} // namespace lang

} // namespace banjo
