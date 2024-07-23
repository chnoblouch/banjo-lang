#include "sir.hpp"

#include "banjo/utils/macros.hpp"

namespace banjo {

namespace lang {

namespace sir {

bool Expr::is_value() const {
    return get_type();
}

Expr Expr::get_type() const {
    if (auto int_literal = match<IntLiteral>()) return int_literal->type;
    else if (auto fp_literal = match<FPLiteral>()) return fp_literal->type;
    else if (auto bool_literal = match<BoolLiteral>()) return bool_literal->type;
    else if (auto char_literal = match<CharLiteral>()) return char_literal->type;
    else if (auto string_literal = match<StringLiteral>()) return string_literal->type;
    else if (auto struct_literal = match<StructLiteral>()) return struct_literal->type;
    else if (auto symbol_expr = match<SymbolExpr>()) return symbol_expr->type;
    else if (auto binary_expr = match<BinaryExpr>()) return binary_expr->type;
    else if (auto unary_expr = match<UnaryExpr>()) return unary_expr->type;
    else if (auto cast_expr = match<CastExpr>()) return cast_expr->type;
    else if (auto call_expr = match<CallExpr>()) return call_expr->type;
    else if (auto field_expr = match<FieldExpr>()) return field_expr->type;
    else if (auto index_expr = match<IndexExpr>()) return index_expr->type;
    else return nullptr;
}

bool Expr::is_type() const {
    return is<PrimitiveType>() || is<PointerType>() || is<FuncType>();
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

const std::string &Symbol::get_name() const {
    if (auto func_def = match<FuncDef>()) return func_def->ident.value;
    else if (auto native_func_decl = match<NativeFuncDecl>()) return native_func_decl->ident.value;
    else if (auto struct_def = match<StructDef>()) return struct_def->ident.value;
    else if (auto struct_field = match<StructField>()) return struct_field->ident.value;
    else if (auto var_decl = match<VarDecl>()) return var_decl->ident.value;
    else if (auto var_stmt = match<VarStmt>()) return var_stmt->name.value;
    else if (auto param = match<Param>()) return param->name.value;
    else ASSERT_UNREACHABLE;
}

Expr Symbol::get_type() {
    if (auto func_def = match<FuncDef>()) return &func_def->type;
    else if (auto native_func_decl = match<NativeFuncDecl>()) return &native_func_decl->type;
    else if (auto struct_field = match<StructField>()) return struct_field->type;
    else if (auto var_decl = match<VarDecl>()) return var_decl->type;
    else if (auto var_stmt = match<VarStmt>()) return var_stmt->type;
    else if (auto param = match<Param>()) return param->type;
    else return nullptr;
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

bool FuncDef::is_main() const {
    return ident.value == "main";
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
