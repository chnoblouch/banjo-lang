#include "sir.hpp"

#include "banjo/sir/sir_visitor.hpp"
#include "banjo/utils/macros.hpp"
#include "sir_visitor.hpp"

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
        SIR_VISIT_IMPOSSIBLE,
        return inner->value == other.as<sir::IntLiteral>().value,
        SIR_VISIT_IMPOSSIBLE,
        SIR_VISIT_IMPOSSIBLE,
        SIR_VISIT_IMPOSSIBLE,
        SIR_VISIT_IMPOSSIBLE,
        SIR_VISIT_IMPOSSIBLE,
        SIR_VISIT_IMPOSSIBLE,
        SIR_VISIT_IMPOSSIBLE,
        SIR_VISIT_IMPOSSIBLE,
        SIR_VISIT_IMPOSSIBLE,
        return inner->symbol == other.as<SymbolExpr>().symbol,
        SIR_VISIT_IMPOSSIBLE,
        SIR_VISIT_IMPOSSIBLE,
        SIR_VISIT_IMPOSSIBLE,
        SIR_VISIT_IMPOSSIBLE,
        SIR_VISIT_IMPOSSIBLE,
        SIR_VISIT_IMPOSSIBLE,
        SIR_VISIT_IMPOSSIBLE,
        return *inner == other.as<TupleExpr>(),
        return *inner == other.as<PrimitiveType>(),
        return *inner == other.as<PointerType>(),
        return *inner == other.as<StaticArrayType>(),
        return *inner == other.as<FuncType>(),
        return *inner == other.as<OptionalType>(),
        SIR_VISIT_IMPOSSIBLE,
        SIR_VISIT_IMPOSSIBLE,
        SIR_VISIT_IMPOSSIBLE,
        SIR_VISIT_IMPOSSIBLE,
        SIR_VISIT_IMPOSSIBLE,
        SIR_VISIT_IMPOSSIBLE,
        SIR_VISIT_IMPOSSIBLE
    );
}

bool Expr::is_value() const {
    return get_type();
}

Expr Expr::get_type() const {
    SIR_VISIT_EXPR(
        *this,
        return nullptr,
        return inner->type,
        return inner->type,
        return inner->type,
        return inner->type,
        return inner->type,
        return inner->type,
        return inner->type,
        return inner->type,
        return inner->type,
        return inner->type,
        return inner->type,
        return inner->type,
        return inner->type,
        return inner->type,
        return inner->type,
        return inner->type,
        return inner->type,
        return nullptr,
        return inner->type,
        return nullptr,
        return nullptr,
        return nullptr,
        return nullptr,
        return nullptr,
        return nullptr,
        return nullptr,
        return nullptr,
        return nullptr,
        SIR_VISIT_IMPOSSIBLE,
        SIR_VISIT_IMPOSSIBLE,
        SIR_VISIT_IMPOSSIBLE
    );
}

bool Expr::is_type() const {
    if (auto symbol_expr = match<SymbolExpr>()) {
        const Symbol &symbol = symbol_expr->symbol;
        return symbol.is<StructDef>() || symbol.is<EnumDef>();
    } else if (auto tuple_expr = match<TupleExpr>()) {
        return tuple_expr->exprs.size() > 0 && tuple_expr->exprs[0].is_type();
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
        return inner->ast_node
    );
}

DeclBlock *Expr::get_decl_block() {
    if (auto symbol_expr = match<SymbolExpr>()) {
        if (auto mod = symbol_expr->symbol.match<Module>()) return &mod->block;
        else if (auto struct_def = symbol_expr->symbol.match<StructDef>()) return &struct_def->block;
        else if (auto enum_def = symbol_expr->symbol.match<EnumDef>()) return &enum_def->block;
        else return nullptr;
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
        SIR_VISIT_IMPOSSIBLE,
        SIR_VISIT_IMPOSSIBLE,
        return inner->ident,
        return inner->ident,
        return inner->ident,
        return inner->ident,
        return inner->ident,
        return inner->ident,
        return inner->ident,
        return inner->ident,
        return inner->ident,
        return inner->ident,
        SIR_VISIT_IMPOSSIBLE,
        return inner->name,
        return inner->name,
        SIR_VISIT_IMPOSSIBLE
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
        return nullptr,
        return nullptr,
        return &inner->type,
        return &inner->type,
        return inner->type,
        return nullptr,
        return inner->type,
        return inner->type,
        return inner->type,
        return nullptr,
        return inner->type,
        return nullptr,
        return nullptr,
        return inner->type,
        return inner->type,
        return nullptr
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
