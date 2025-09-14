#include "sir.hpp"

#include "banjo/sir/sir_visitor.hpp"
#include "banjo/utils/macros.hpp"

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
        return true,                                              // empty
        return inner->value == other.as<sir::IntLiteral>().value, // int_literal
        return false,                                             // fp_literal
        return false,                                             // bool_literal
        return false,                                             // char_literal
        return false,                                             // null_literal
        return false,                                             // none_literal
        return false,                                             // undefined_literal
        return false,                                             // array_literal
        return false,                                             // string_literal
        return false,                                             // struct_literal
        return false,                                             // union_case_literal
        return false,                                             // map_literal
        return false,                                             // closure_literal
        return inner->symbol == other.as<SymbolExpr>().symbol,    // symbol_expr
        return false,                                             // binary_expr
        return false,                                             // unary_expr
        return false,                                             // cast_expr
        return false,                                             // index_expr
        return false,                                             // call_expr
        return false,                                             // field_expr
        return false,                                             // range_expr
        return *inner == other.as<TupleExpr>(),                   // tuple_expr
        return false,                                             // coercion_expr
        return *inner == other.as<PrimitiveType>(),               // primitive_type
        return *inner == other.as<PointerType>(),                 // pointer_type
        return *inner == other.as<StaticArrayType>(),             // static_array_type
        return *inner == other.as<FuncType>(),                    // func_type
        return *inner == other.as<OptionalType>(),                // optional_type
        return *inner == other.as<ResultType>(),                  // result_type
        return *inner == other.as<ArrayType>(),                   // array_type
        return *inner == other.as<MapType>(),                     // map_type
        return *inner == other.as<ClosureType>(),                 // closure_type
        return *inner == other.as<ReferenceType>(),               // reference_type
        return false,                                             // ident_expr
        return false,                                             // star_expr
        return false,                                             // bracket_expr
        return false,                                             // dot_expr
        return false,                                             // pseudo_type
        return false,                                             // meta_access
        return false,                                             // meta_field_expr
        return false,                                             // meta_call_expr
        return false,                                             // init_expr
        return false,                                             // move_expr
        return false,                                             // deinit_expr
        return true                                               // error
    );
}

bool Expr::is_value() const {
    return get_type();
}

Expr Expr::get_type() const {
    SIR_VISIT_EXPR(
        *this,
        return nullptr,     // empty
        return inner->type, // int_literal
        return inner->type, // fp_literal
        return inner->type, // bool_literal
        return inner->type, // char_literal
        return inner->type, // null_literal
        return inner->type, // none_literal
        return inner->type, // undefined_literal
        return inner->type, // array_literal
        return inner->type, // string_literal
        return inner->type, // struct_literal
        return inner->type, // union_case_literal
        return inner->type, // map_literal
        return inner->type, // closure_literal
        return inner->type, // symbol_expr
        return inner->type, // binary_expr
        return inner->type, // unary_expr
        return inner->type, // cast_expr
        return inner->type, // index_expr
        return inner->type, // call_expr
        return inner->type, // field_expr
        return nullptr,     // range_expr
        return inner->type, // tuple_expr
        return inner->type, // coercion_type
        return nullptr,     // primitive_type
        return nullptr,     // pointer_type
        return nullptr,     // static_array_type
        return nullptr,     // func_type
        return nullptr,     // optional_type
        return nullptr,     // result_type
        return nullptr,     // array_type
        return nullptr,     // map_type
        return nullptr,     // closure_type
        return nullptr,     // reference_type
        return nullptr,     // ident_expr
        return nullptr,     // star_expr
        return nullptr,     // bracket_expr
        return nullptr,     // dot_expr
        return nullptr,     // pseudo_type
        return nullptr,     // meta_access
        return nullptr,     // meta_field_expr
        return nullptr,     // meta_call_expr
        return inner->type, // init_expr
        return inner->type, // move_expr
        return inner->type, // deinit_expr
        return nullptr      // error
    );
}

ExprCategory Expr::get_category() const {
    if (is<MetaAccess>()) {
        return ExprCategory::META_ACCESS;
    } else if (auto symbol_expr = match<SymbolExpr>()) {
        if (symbol_expr->symbol.is_one_of<StructDef, EnumDef, UnionDef, UnionCase, ProtoDef>()) {
            return ExprCategory::TYPE;
        } else if (symbol_expr->symbol.is<Module>()) {
            return ExprCategory::MODULE;
        } else {
            return ExprCategory::VALUE;
        }
    } else if (auto tuple_expr = match<TupleExpr>()) {
        if (tuple_expr->exprs.empty()) {
            return ExprCategory::VALUE_OR_TYPE;
        } else {
            return tuple_expr->exprs[0].get_category();
        }
    } else if (auto star_expr = match<StarExpr>()) {
        return star_expr->value.get_category();
    } else if (is<PrimitiveType>() || is<PointerType>() || is<StaticArrayType>() || is<FuncType>() ||
               is<sir::ClosureType>() || is<sir::ReferenceType>()) {
        return ExprCategory::TYPE;
    } else {
        return ExprCategory::VALUE;
    }
}

bool Expr::is_type() const {
    if (auto symbol_expr = match<SymbolExpr>()) {
        return symbol_expr->symbol.is_one_of<StructDef, EnumDef, UnionDef, UnionCase, ProtoDef>();
    } else if (auto tuple_expr = match<TupleExpr>()) {
        return tuple_expr->exprs.empty() || tuple_expr->exprs[0].is_type();
    } else if (auto star_expr = match<StarExpr>()) {
        return star_expr->value.is_type();
    } else {
        return is<PrimitiveType>() || is<PointerType>() || is<StaticArrayType>() || is<FuncType>() ||
               is<sir::ClosureType>() || is<sir::ReferenceType>();
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
            case Primitive::U64:
            case Primitive::USIZE: return true;
            default: return false;
        }
    } else if (is_pseudo_type(sir::PseudoTypeKind::INT_LITERAL)) {
        return true;
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
            case Primitive::U64:
            case Primitive::USIZE: return true;
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

const ProtoDef *Expr::match_proto_ptr() const {
    if (auto pointer_type = match<PointerType>()) {
        return pointer_type->base_type.match_symbol<sir::ProtoDef>();
    } else {
        return nullptr;
    }
}

ProtoDef *Expr::match_proto_ptr() {
    return const_cast<ProtoDef *>(std::as_const(*this).match_proto_ptr());
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

bool Expr::is_pseudo_type(PseudoTypeKind kind) const {
    if (auto pseudo_type = match<PseudoType>()) {
        return pseudo_type->kind == kind;
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

ASTNode *Stmt::get_ast_node() const {
    SIR_VISIT_STMT(
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
        SIR_VISIT_IMPOSSIBLE,
        return inner->get_ast_node(),
        return inner->ast_node,
        return inner->ast_node
    );
}

const Ident &Symbol::get_ident() const {
    SIR_VISIT_SYMBOL(
        *this,
        SIR_VISIT_IMPOSSIBLE, // empty
        SIR_VISIT_IMPOSSIBLE, // module
        return inner->ident,  // func_def
        return inner->ident,  // func_decl
        return inner->ident,  // native_func_decl
        return inner->ident,  // const_def
        return inner->ident,  // struct_def
        return inner->ident,  // struct_field
        return inner->ident,  // var_decl
        return inner->ident,  // native_var_decl
        return inner->ident,  // enum_def
        return inner->ident,  // enum_variant
        return inner->ident,  // union_def
        return inner->ident,  // union_case
        return inner->ident,  // proto_def
        return inner->ident,  // type_alias
        return inner->ident,  // use_ident
        SIR_VISIT_IMPOSSIBLE, // use_rebind
        return inner->name,   // local
        return inner->name,   // param
        SIR_VISIT_IMPOSSIBLE  // overload_set
    );
}

Ident &Symbol::get_ident() {
    return const_cast<Ident &>(std::as_const(*this).get_ident());
}

std::string Symbol::get_name() const {
    if (auto mod = match<Module>()) {
        return std::string{mod->path.to_string()};
    } else {
        return std::string{get_ident().value};
    }
}

Symbol Symbol::get_parent() const {
    if (auto func_def = match<FuncDef>()) return func_def->parent;
    else if (auto struct_def = match<StructDef>()) return struct_def->parent;
    else return nullptr;
}

Expr Symbol::get_type() {
    SIR_VISIT_SYMBOL(
        *this,
        return nullptr,      // empty
        return nullptr,      // module
        return &inner->type, // func_def
        return &inner->type, // func_decl
        return &inner->type, // native_func_decl
        return inner->type,  // const_def
        return nullptr,      // struct_def
        return inner->type,  // struct_field
        return inner->type,  // var_decl
        return inner->type,  // native_var_decl
        return nullptr,      // enum_def
        return inner->type,  // enum_variant
        return nullptr,      // union_def
        return nullptr,      // union_case
        return nullptr,      // proto_def
        return nullptr,      // type_alias
        return nullptr,      // use_ident
        return nullptr,      // use_rebind
        return inner->type,  // local
        return inner->type,  // param
        return nullptr       // overload_set
    );
}

Symbol Symbol::resolve() const {
    if (auto use_ident = match<UseIdent>()) return use_ident->symbol;
    else if (auto use_rebind = match<UseRebind>()) return use_rebind->symbol;
    else return *this;
}

const SymbolTable *Symbol::get_symbol_table() const {
    const DeclBlock *block = get_decl_block();
    return block ? block->symbol_table : nullptr;
}

SymbolTable *Symbol::get_symbol_table() {
    return const_cast<SymbolTable *>(std::as_const(*this).get_symbol_table());
}

const DeclBlock *Symbol::get_decl_block() const {
    if (auto mod = match<Module>()) return &mod->block;
    else if (auto struct_def = match<StructDef>()) return &struct_def->block;
    else if (auto enum_def = match<EnumDef>()) return &enum_def->block;
    else if (auto union_def = match<UnionDef>()) return &union_def->block;
    else if (auto proto_def = match<ProtoDef>()) return &proto_def->block;
    else return nullptr;
}

DeclBlock *Symbol::get_decl_block() {
    return const_cast<DeclBlock *>(std::as_const(*this).get_decl_block());
}

void SymbolTable::insert_decl(std::string_view name, Symbol symbol) {
    ASSERT(!(symbol.is_one_of<Local, Param>()));

    symbols.insert({name, symbol});
}

void SymbolTable::insert_local(std::string_view name, Symbol symbol) {
    ASSERT((symbol.is_one_of<Local, Param>()));

    symbols.insert({name, symbol});
    local_symbols_ordered.push_back(symbol);
}

Symbol SymbolTable::look_up(std::string_view name) const {
    auto iter = symbols.find(name);
    if (iter == symbols.end()) {
        return parent ? parent->look_up(name) : nullptr;
    } else {
        return iter->second.resolve();
    }
}

Symbol SymbolTable::look_up_local(std::string_view name) const {
    auto iter = symbols.find(name);
    return iter == symbols.end() ? nullptr : iter->second.resolve();
}

bool PseudoType::is_struct_by_default() const {
    switch (kind) {
        case PseudoTypeKind::ARRAY_LITERAL:
        case PseudoTypeKind::STRING_LITERAL: return true;
        default: return false;
    }
}

Module &FuncDef::find_mod() const {
    Symbol symbol = parent;

    while (!symbol.is<sir::Module>()) {
        symbol = symbol.get_parent();
    }

    return symbol.as<Module>();
}

Module &StructDef::find_mod() const {
    Symbol symbol = parent;

    while (!symbol.is<sir::Module>()) {
        symbol = symbol.get_parent();
    }

    return symbol.as<Module>();
}

StructField *StructDef::find_field(std::string_view name) const {
    for (StructField *field : fields) {
        if (field->ident.value == name) {
            return field;
        }
    }

    return nullptr;
}

bool StructDef::has_impl_for(const sir::ProtoDef &proto_def) const {
    for (const sir::Expr &impl : impls) {
        if (auto impl_proto_def = impl.match_symbol<sir::ProtoDef>()) {
            if (impl_proto_def == &proto_def) {
                return true;
            }
        }
    }

    return false;
}

Attributes::Layout StructDef::get_layout() const {
    if (!attrs || !attrs->layout) {
        return Attributes::Layout::DEFAULT;
    }

    return *attrs->layout;
}

unsigned UnionDef::get_index(const UnionCase &case_) const {
    for (unsigned i = 0; i < cases.size(); i++) {
        if (cases[i] == &case_) {
            return i;
        }
    }

    ASSERT_UNREACHABLE;
}

std::optional<unsigned> UnionCase::find_field(std::string_view name) const {
    for (unsigned i = 0; i < fields.size(); i++) {
        if (fields[i].ident.value == name) {
            return i;
        }
    }

    return {};
}

std::optional<unsigned> ProtoDef::get_index(std::string_view name) const {
    for (unsigned i = 0; i < func_decls.size(); i++) {
        if (func_decls[i].get_ident().value == name) {
            return i;
        }
    }

    return {};
}

ASTNode *ProtoFuncDecl::get_ast_node() const {
    if (auto func_decl = decl.match<sir::FuncDecl>()) {
        return func_decl->ast_node;
    } else if (auto func_def = decl.match<sir::FuncDef>()) {
        return func_def->ast_node;
    } else {
        ASSERT_UNREACHABLE;
    }
}

const Ident &ProtoFuncDecl::get_ident() const {
    if (auto func_decl = decl.match<sir::FuncDecl>()) {
        return func_decl->ident;
    } else if (auto func_def = decl.match<sir::FuncDef>()) {
        return func_def->ident;
    } else {
        ASSERT_UNREACHABLE;
    }
}

Symbol ProtoFuncDecl::get_parent() const {
    if (auto func_decl = decl.match<sir::FuncDecl>()) {
        return func_decl->parent;
    } else if (auto func_def = decl.match<sir::FuncDef>()) {
        return func_def->parent;
    } else {
        ASSERT_UNREACHABLE;
    }
}

FuncType &ProtoFuncDecl::get_type() {
    if (auto func_decl = decl.match<sir::FuncDecl>()) {
        return func_decl->type;
    } else if (auto func_def = decl.match<sir::FuncDef>()) {
        return func_def->type;
    } else {
        ASSERT_UNREACHABLE;
    }
}

} // namespace sir

} // namespace lang

} // namespace banjo
