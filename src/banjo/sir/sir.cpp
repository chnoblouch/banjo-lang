#include "sir.hpp"

namespace banjo {

namespace lang {

namespace sir {

Expr Expr::get_type() const {
    if (auto int_literal = match<IntLiteral>()) return int_literal->type;
    else if (auto char_literal = match<CharLiteral>()) return char_literal->type;
    else if (auto string_literal = match<StringLiteral>()) return string_literal->type;
    else if (auto struct_literal = match<StructLiteral>()) return struct_literal->type;
    else if (auto ident_expr = match<IdentExpr>()) return ident_expr->type;
    else if (auto binary_expr = match<BinaryExpr>()) return binary_expr->type;
    else if (auto unary_expr = match<UnaryExpr>()) return unary_expr->type;
    else if (auto call_expr = match<CallExpr>()) return call_expr->type;
    else if (auto dot_expr = match<DotExpr>()) return dot_expr->type;
    else if (auto star_expr = match<StarExpr>()) return star_expr->type;
    else return nullptr;
}

bool Expr::is_type() const {
    return is<PrimitiveType>();
}

Expr Symbol::get_type() {
    if (auto func_def = match<FuncDef>()) return &func_def->type;
    else if (auto native_func_decl = match<NativeFuncDecl>()) return &native_func_decl->type;
    else if (auto var_stmt = match<VarStmt>()) return var_stmt->type;
    else if (auto param = match<Param>()) return param->type;
    else return nullptr;
}

Symbol SymbolTable::look_up(std::string_view name) {
    auto iter = symbols.find(name);
    if (iter == symbols.end()) {
        return parent ? parent->look_up(name) : nullptr;
    } else {
        return iter->second;
    }
}

bool FuncDef::is_main() const {
    return ident.value == "main";
}

} // namespace sir

} // namespace lang

} // namespace banjo
