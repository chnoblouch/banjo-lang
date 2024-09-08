#ifndef SIR_H
#define SIR_H

#include "banjo/ast/attribute.hpp"
#include "banjo/symbol/module_path.hpp"
#include "banjo/utils/growable_arena.hpp"
#include "banjo/utils/large_int.hpp"
#include "banjo/utils/macros.hpp"

#include <concepts>
#include <cstddef>
#include <list>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

namespace banjo {

namespace lang {

class ASTNode;

namespace sir {

template <class... Ts>
struct Visitor : Ts... {
    using Ts::operator()...;
};

struct IntLiteral;
struct FPLiteral;
struct BoolLiteral;
struct CharLiteral;
struct NullLiteral;
struct NoneLiteral;
struct UndefinedLiteral;
struct ArrayLiteral;
struct StringLiteral;
struct StructLiteral;
struct ClosureLiteral;
struct SymbolExpr;
struct BinaryExpr;
struct UnaryExpr;
struct CastExpr;
struct IndexExpr;
struct CallExpr;
struct FieldExpr;
struct RangeExpr;
struct TupleExpr;
enum class Primitive;
struct PrimitiveType;
struct PointerType;
struct StaticArrayType;
struct FuncType;
struct OptionalType;
struct ArrayType;
struct ClosureType;
struct IdentExpr;
struct StarExpr;
struct BracketExpr;
struct DotExpr;
struct PseudoType;
struct MetaAccess;
struct MetaFieldExpr;
struct MetaCallExpr;
struct VarStmt;
struct AssignStmt;
struct CompAssignStmt;
struct ReturnStmt;
struct IfStmt;
struct WhileStmt;
struct ForStmt;
struct LoopStmt;
struct ContinueStmt;
struct BreakStmt;
struct MetaIfStmt;
struct ExpandedMetaStmt;
struct FuncDef;
struct NativeFuncDecl;
struct ConstDef;
struct StructDef;
struct StructField;
struct VarDecl;
struct NativeVarDecl;
struct EnumDef;
struct EnumVariant;
struct TypeAlias;
struct Param;
struct UseDecl;
struct UseIdent;
struct UseDotExpr;
struct UseRebind;
struct UseList;

struct Unit;
struct DeclBlock;
struct Block;
struct SymbolTable;
struct OverloadSet;
struct Ident;
struct Module;

class Expr;
class Stmt;
class Decl;
class Symbol;
class UseItem;

class Expr {
    std::variant<
        IntLiteral *,
        FPLiteral *,
        BoolLiteral *,
        CharLiteral *,
        NullLiteral *,
        NoneLiteral *,
        UndefinedLiteral *,
        ArrayLiteral *,
        StringLiteral *,
        StructLiteral *,
        ClosureLiteral *,
        SymbolExpr *,
        BinaryExpr *,
        UnaryExpr *,
        CastExpr *,
        IndexExpr *,
        CallExpr *,
        FieldExpr *,
        RangeExpr *,
        TupleExpr *,
        PrimitiveType *,
        PointerType *,
        StaticArrayType *,
        FuncType *,
        OptionalType *,
        ArrayType *,
        ClosureType *,
        IdentExpr *,
        StarExpr *,
        BracketExpr *,
        DotExpr *,
        PseudoType *,
        MetaAccess *,
        MetaFieldExpr *,
        MetaCallExpr *,
        std::nullptr_t>
        kind;

public:
    Expr() : kind(nullptr) {}

    template <typename T>
    Expr(T value) : kind(std::move(value)) {}

    template <typename T>
    bool is() const {
        return std::holds_alternative<T *>(kind);
    }

    template <typename T>
    bool is_symbol() const;

    template <typename T>
    T &as() {
        return *std::get<T *>(kind);
    }

    template <typename T>
    const T &as() const {
        return *std::get<T *>(kind);
    }

    template <typename T>
    T &as_symbol();

    template <typename T>
    T *match() {
        auto result = std::get_if<T *>(&kind);
        return result ? *result : nullptr;
    }

    template <typename T>
    const T *match() const {
        auto result = std::get_if<T *>(&kind);
        return result ? *result : nullptr;
    }

    template <typename T>
    T *match_symbol();

    template <typename T>
    const T *match_symbol() const;

    operator bool() const { return !std::holds_alternative<std::nullptr_t>(kind); }
    bool operator==(const Expr &other) const;
    bool operator!=(const Expr &other) const { return !(*this == other); }

    bool is_value() const;
    Expr get_type() const;

    bool is_type() const;
    bool is_primitive_type(Primitive primitive) const;
    bool is_int_type() const;
    bool is_signed_type() const;
    bool is_unsigned_type() const;
    bool is_fp_type() const;
    bool is_addr_like_type() const;
    bool is_u8_ptr() const;
    bool is_symbol(sir::Symbol symbol) const;

    ASTNode *get_ast_node() const;
    DeclBlock *get_decl_block();
};

class Stmt {

private:
    std::variant<
        VarStmt *,
        AssignStmt *,
        CompAssignStmt *,
        ReturnStmt *,
        IfStmt *,
        WhileStmt *,
        ForStmt *,
        LoopStmt *,
        ContinueStmt *,
        BreakStmt *,
        MetaIfStmt *,
        ExpandedMetaStmt *,
        Expr *,
        Block *,
        std::nullptr_t>
        kind;

public:
    Stmt() : kind(nullptr) {}

    template <typename T>
    Stmt(T kind) : kind(kind) {}

    template <typename T>
    bool is() const {
        return std::holds_alternative<T *>(kind);
    }

    template <typename T>
    T &as() {
        return *std::get<T *>(kind);
    }

    template <typename T>
    T *match() {
        auto result = std::get_if<T *>(&kind);
        return result ? *result : nullptr;
    }

    template <typename T>
    const T *match() const {
        auto result = std::get_if<T *>(&kind);
        return result ? *result : nullptr;
    }

    operator bool() const { return !std::holds_alternative<std::nullptr_t>(kind); }
};

class Decl {

private:
    std::variant<
        FuncDef *,
        NativeFuncDecl *,
        ConstDef *,
        StructDef *,
        StructField *,
        VarDecl *,
        NativeVarDecl *,
        EnumDef *,
        EnumVariant *,
        TypeAlias *,
        UseDecl *,
        MetaIfStmt *,
        ExpandedMetaStmt *,
        std::nullptr_t>
        kind;

public:
    Decl() : kind(nullptr) {}

    template <typename T>
    Decl(T kind) : kind(kind) {}

    template <typename T>
    bool is() const {
        return std::holds_alternative<T *>(kind);
    }

    template <typename T>
    T &as() {
        return *std::get<T *>(kind);
    }

    template <typename T>
    const T &as() const {
        return *std::get<T *>(kind);
    }

    template <typename T>
    T *match() {
        auto result = std::get_if<T *>(&kind);
        return result ? *result : nullptr;
    }

    template <typename T>
    const T *match() const {
        auto result = std::get_if<T *>(&kind);
        return result ? *result : nullptr;
    }

    operator bool() const { return !std::holds_alternative<std::nullptr_t>(kind); }
};

class Symbol {
    std::variant<
        Module *,
        FuncDef *,
        NativeFuncDecl *,
        ConstDef *,
        StructDef *,
        StructField *,
        VarDecl *,
        NativeVarDecl *,
        EnumDef *,
        EnumVariant *,
        TypeAlias *,
        UseIdent *,
        UseRebind *,
        VarStmt *,
        Param *,
        OverloadSet *,
        std::nullptr_t>
        kind;

public:
    Symbol() : kind(nullptr) {}

    template <typename T>
    Symbol(T value) : kind(std::move(value)) {}

    template <typename T>
    bool is() const {
        return std::holds_alternative<T *>(kind);
    }

    template <typename T>
    T &as() {
        return *std::get<T *>(kind);
    }

    template <typename T>
    const T &as() const {
        return *std::get<T *>(kind);
    }

    template <typename T>
    T *match() {
        auto result = std::get_if<T *>(&kind);
        return result ? *result : nullptr;
    }

    template <typename T>
    const T *match() const {
        auto result = std::get_if<T *>(&kind);
        return result ? *result : nullptr;
    }

    operator bool() const { return !std::holds_alternative<std::nullptr_t>(kind); }
    bool operator==(const Symbol &other) const;
    bool operator!=(const Symbol &other) const { return !(*this == other); }

    const Ident &get_ident() const;
    Ident &get_ident();
    std::string get_name() const;
    Expr get_type();
    Symbol resolve();
    SymbolTable *get_symbol_table();
    DeclBlock *get_decl_block();
};

class UseItem {
    std::variant<UseIdent *, UseRebind *, UseDotExpr *, UseList *> kind;

public:
    UseItem() {}

    template <typename T>
    UseItem(T value) : kind(std::move(value)) {}

    template <typename T>
    T *match() {
        auto result = std::get_if<T *>(&kind);
        return result ? *result : nullptr;
    }

    template <typename T>
    const T *match() const {
        auto result = std::get_if<T *>(&kind);
        return result ? *result : nullptr;
    }
};

class Node {

private:
    std::variant<Expr, Stmt, Decl> kind;

public:
    Node() {}

    template <typename T>
    Node(T kind) : kind(kind) {}

    template <typename T>
    T &as() {
        return std::get<T>(kind);
    }

    template <typename T>
    const T &as() const {
        return std::get<T>(kind);
    }

    template <typename T>
    T *match() {
        return std::get_if<T>(&kind);
    }

    template <typename T>
    const T *match() const {
        return std::get_if<T>(&kind);
    }
};

struct DeclBlock {
    ASTNode *ast_node;
    std::vector<Decl> decls;
    SymbolTable *symbol_table;
};

struct Block {
    ASTNode *ast_node;
    std::vector<Stmt> stmts;
    SymbolTable *symbol_table;
};

struct SymbolTable {
    SymbolTable *parent;
    std::unordered_map<std::string_view, Symbol> symbols;
    bool complete = false;

    Symbol look_up(std::string_view name);
};

struct OverloadSet {
    std::vector<FuncDef *> func_defs;
};

struct Attributes {
    bool exposed = false;
    bool dllexport = false;
    std::optional<std::string> link_name = {};
};

struct Ident {
    ASTNode *ast_node;
    std::string value;
};

struct Param {
    ASTNode *ast_node;
    Ident name;
    Expr type;

    bool operator==(const Param &other) const { return type == other.type; }
    bool operator!=(const Param &other) const { return !(*this == other); }
};

struct GenericParam {
    ASTNode *ast_node;
    Ident ident;
};

template <typename T>
struct Specialization {
    std::vector<Expr> args;
    T *def;
};

struct FuncType {
    ASTNode *ast_node;
    std::vector<Param> params;
    Expr return_type;

    bool operator==(const FuncType &other) const { return params == other.params && return_type == other.return_type; }
    bool operator!=(const FuncType &other) const { return !(*this == other); }
};

struct IntLiteral {
    ASTNode *ast_node;
    Expr type;
    LargeInt value;
};

struct FPLiteral {
    ASTNode *ast_node;
    Expr type;
    double value;
};

struct BoolLiteral {
    ASTNode *ast_node;
    Expr type;
    bool value;
};

struct CharLiteral {
    ASTNode *ast_node;
    Expr type;
    char value;
};

struct NullLiteral {
    ASTNode *ast_node;
    Expr type;
};

struct NoneLiteral {
    ASTNode *ast_node;
    Expr type;
};

struct UndefinedLiteral {
    ASTNode *ast_node;
    Expr type;
};

struct ArrayLiteral {
    ASTNode *ast_node;
    Expr type;
    std::vector<Expr> values;
};

struct StringLiteral {
    ASTNode *ast_node;
    Expr type;
    std::string value;
};

struct StructLiteralEntry {
    Ident ident;
    Expr value;
    StructField *field;
};

struct StructLiteral {
    ASTNode *ast_node;
    Expr type;
    std::vector<StructLiteralEntry> entries;
};

struct ClosureLiteral {
    ASTNode *ast_node;
    Expr type;
    FuncType func_type;
    Block block;
};

struct SymbolExpr {
    ASTNode *ast_node;
    Expr type;
    Symbol symbol;

    bool operator==(const SymbolExpr &other) const { return symbol == other.symbol; }
    bool operator!=(const SymbolExpr &other) const { return !(*this == other); }
};

enum class BinaryOp {
    ADD,
    SUB,
    MUL,
    DIV,
    MOD,
    BIT_AND,
    BIT_OR,
    BIT_XOR,
    SHL,
    SHR,
    EQ,
    NE,
    GT,
    LT,
    GE,
    LE,
    AND,
    OR,
};

struct BinaryExpr {
    ASTNode *ast_node;
    Expr type;
    BinaryOp op;
    Expr lhs;
    Expr rhs;
};

enum class UnaryOp {
    NEG,
    REF,
    DEREF,
    NOT,
};

struct UnaryExpr {
    ASTNode *ast_node;
    Expr type;
    UnaryOp op;
    Expr value;
};

struct CastExpr {
    ASTNode *ast_node;
    Expr type;
    Expr value;
};

struct IndexExpr {
    ASTNode *ast_node;
    Expr type;
    Expr base;
    Expr index;
};

struct CallExpr {
    ASTNode *ast_node;
    Expr type;
    Expr callee;
    std::vector<Expr> args;
};

struct FieldExpr {
    ASTNode *ast_node;
    Expr type;
    Expr base;
    unsigned field_index;
};

struct RangeExpr {
    ASTNode *ast_node;
    Expr lhs;
    Expr rhs;
};

struct TupleExpr {
    ASTNode *ast_node;
    Expr type;
    std::vector<Expr> exprs;

    bool operator==(const TupleExpr &other) const { return exprs == other.exprs; }
    bool operator!=(const TupleExpr &other) const { return !(*this == other); }
};

enum class Primitive {
    I8,
    I16,
    I32,
    I64,
    U8,
    U16,
    U32,
    U64,
    USIZE = U64,
    F32,
    F64,
    BOOL,
    ADDR,
    VOID,
};

struct PrimitiveType {
    ASTNode *ast_node;
    Primitive primitive;

    bool operator==(const PrimitiveType &other) const { return primitive == other.primitive; }
    bool operator!=(const PrimitiveType &other) const { return !(*this == other); }
};

struct PointerType {
    ASTNode *ast_node;
    Expr base_type;

    bool operator==(const PointerType &other) const { return base_type == other.base_type; }
    bool operator!=(const PointerType &other) const { return !(*this == other); }
};

struct StaticArrayType {
    ASTNode *ast_node;
    Expr base_type;
    Expr length;

    bool operator==(const StaticArrayType &other) const {
        return base_type == other.base_type && length == other.length;
    }

    bool operator!=(const StaticArrayType &other) const { return !(*this == other); }
};

struct OptionalType {
    ASTNode *ast_node;
    Expr base_type;

    bool operator==(const OptionalType &other) const { return base_type == other.base_type; }
    bool operator!=(const OptionalType &other) const { return !(*this == other); }
};

struct ArrayType {
    ASTNode *ast_node;
    Expr base_type;

    bool operator==(const ArrayType &other) const { return base_type == other.base_type; }
    bool operator!=(const ArrayType &other) const { return !(*this == other); }
};

struct ClosureType {
    ASTNode *ast_node;
    FuncType func_type;
    StructDef *underlying_struct;

    bool operator==(const ClosureType &other) const { return func_type == other.func_type; }
    bool operator!=(const ClosureType &other) const { return !(*this == other); }
};

struct IdentExpr {
    ASTNode *ast_node;
    std::string value;
};

struct StarExpr {
    ASTNode *ast_node;
    Expr value;
};

struct BracketExpr {
    ASTNode *ast_node;
    Expr lhs;
    std::vector<Expr> rhs;
};

struct DotExpr {
    ASTNode *ast_node;
    Expr lhs;
    Ident rhs;
};

enum class PseudoTypeKind {
    INT_LITERAL,
    FP_LITERAL,
    STRING_LITERAL,
    ARRAY_LITERAL,
};

struct PseudoType {
    PseudoTypeKind kind;

    bool is_struct_by_default() const;
};

struct MetaAccess {
    ASTNode *ast_node;
    Expr expr;
};

struct MetaFieldExpr {
    ASTNode *ast_node;
    Expr base;
    Ident field;
};

struct MetaCallExpr {
    ASTNode *ast_node;
    Expr callee;
    std::vector<Expr> args;
};

struct VarStmt {
    ASTNode *ast_node;
    Ident name;
    Expr type;
    Expr value;
};

struct AssignStmt {
    ASTNode *ast_node;
    Expr lhs;
    Expr rhs;
};

struct CompAssignStmt {
    ASTNode *ast_node;
    BinaryOp op;
    Expr lhs;
    Expr rhs;
};

struct ReturnStmt {
    ASTNode *ast_node;
    Expr value;
};

struct IfCondBranch {
    ASTNode *ast_node;
    Expr condition;
    Block block;
};

struct IfElseBranch {
    ASTNode *ast_node;
    Block block;
};

struct IfStmt {
    ASTNode *ast_node;
    std::vector<IfCondBranch> cond_branches;
    std::optional<IfElseBranch> else_branch;
};

struct WhileStmt {
    ASTNode *ast_node;
    Expr condition;
    Block block;
};

struct ForStmt {
    ASTNode *ast_node;
    Ident ident;
    Expr range;
    Block block;
};

struct LoopStmt {
    ASTNode *ast_node;
    Expr condition;
    Block block;
    std::optional<Block> latch;
};

struct ContinueStmt {
    ASTNode *ast_node;
};

struct BreakStmt {
    ASTNode *ast_node;
};

struct MetaBlock {
    ASTNode *ast_node;
    std::vector<Node> nodes;
};

struct MetaIfCondBranch {
    ASTNode *ast_node;
    Expr condition;
    MetaBlock block;
};

struct MetaIfElseBranch {
    ASTNode *ast_node;
    MetaBlock block;
};

struct MetaIfStmt {
    ASTNode *ast_node;
    std::vector<MetaIfCondBranch> cond_branches;
    std::optional<MetaIfElseBranch> else_branch;
};

struct ExpandedMetaStmt {};

struct FuncDef {
    ASTNode *ast_node;
    Ident ident;
    FuncType type;
    Block block;
    Attributes *attrs = nullptr;
    std::vector<GenericParam> generic_params;
    std::list<Specialization<FuncDef>> specializations;

    bool is_generic() const { return !generic_params.empty(); }
    bool is_main() const { return ident.value == "main"; }
    bool is_method() const { return type.params.size() > 0 && type.params[0].name.value == "self"; }
};

struct NativeFuncDecl {
    ASTNode *ast_node;
    Ident ident;
    FuncType type;
    Attributes *attrs = nullptr;
};

struct ConstDef {
    ASTNode *ast_node;
    Ident ident;
    Expr type;
    Expr value;
};

struct StructDef {
    ASTNode *ast_node;
    Ident ident;
    DeclBlock block;
    std::vector<StructField *> fields;
    std::vector<GenericParam> generic_params;
    std::list<Specialization<StructDef>> specializations;

    StructField *find_field(std::string_view name) const;
    bool is_generic() const { return !generic_params.empty(); }
};

struct StructField {
    ASTNode *ast_node;
    Ident ident;
    Expr type;
    unsigned index;
};

struct VarDecl {
    ASTNode *ast_node;
    Ident ident;
    Expr type;
};

struct NativeVarDecl {
    ASTNode *ast_node;
    Ident ident;
    Expr type;
    Attributes *attrs = nullptr;
};

struct EnumDef {
    ASTNode *ast_node;
    Ident ident;
    DeclBlock block;
    std::vector<EnumVariant *> variants;
};

struct EnumVariant {
    ASTNode *ast_node;
    Ident ident;
    Expr type;
    Expr value;
};

struct TypeAlias {
    ASTNode *ast_node;
    Ident ident;
    Expr type;
};

struct UseDecl {
    ASTNode *ast_node;
    UseItem root_item;
};

struct UseIdent {
    Ident ident;
    Symbol symbol;
};

struct UseRebind {
    ASTNode *ast_node;
    Ident target_ident;
    Ident local_ident;
    Symbol symbol;
};

struct UseDotExpr {
    ASTNode *ast_node;
    UseItem lhs;
    UseItem rhs;
};

struct UseList {
    ASTNode *ast_node;
    std::vector<UseItem> items;
};

typedef std::variant<
    IntLiteral,
    FPLiteral,
    BoolLiteral,
    CharLiteral,
    NullLiteral,
    NoneLiteral,
    UndefinedLiteral,
    ArrayLiteral,
    StringLiteral,
    StructLiteral,
    ClosureLiteral,
    SymbolExpr,
    BinaryExpr,
    UnaryExpr,
    CastExpr,
    IndexExpr,
    CallExpr,
    FieldExpr,
    RangeExpr,
    TupleExpr,
    PrimitiveType,
    PointerType,
    StaticArrayType,
    FuncType,
    OptionalType,
    ArrayType,
    ClosureType,
    IdentExpr,
    StarExpr,
    BracketExpr,
    DotExpr,
    PseudoType,
    MetaAccess,
    MetaFieldExpr,
    MetaCallExpr>
    ExprStorage;

typedef std::variant<
    VarStmt,
    AssignStmt,
    CompAssignStmt,
    ReturnStmt,
    IfStmt,
    WhileStmt,
    ForStmt,
    LoopStmt,
    ContinueStmt,
    BreakStmt,
    MetaIfStmt,
    ExpandedMetaStmt,
    Expr,
    Block>
    StmtStorage;

typedef std::variant<
    FuncDef,
    NativeFuncDecl,
    ConstDef,
    StructDef,
    StructField,
    VarDecl,
    NativeVarDecl,
    EnumDef,
    EnumVariant,
    TypeAlias,
    UseDecl>
    DeclStorage;

typedef std::variant<UseIdent, UseRebind, UseDotExpr, UseList> UseItemStorage;

struct Module {
    utils::GrowableArena<ExprStorage> expr_arena;
    utils::GrowableArena<StmtStorage> stmt_arena;
    utils::GrowableArena<DeclStorage> decl_arena;
    utils::GrowableArena<UseItemStorage> use_item_arena;
    utils::GrowableArena<SymbolTable> symbol_table_arena;
    utils::GrowableArena<OverloadSet> overload_set_arena;
    utils::GrowableArena<Attributes> attributes_arena;

    ModulePath path;
    DeclBlock block;
    std::vector<Module *> sub_mods;

    template <typename T>
    T *create_expr(T value) {
        return &std::get<T>(*expr_arena.create(value));
    }

    template <typename T>
    T *create_stmt(T value) {
        return &std::get<T>(*stmt_arena.create(std::move(value)));
    }

    template <typename T>
    T *create_decl(T value) {
        return &std::get<T>(*decl_arena.create(std::move(value)));
    }

    template <typename T>
    T *create_use_item(T value) {
        return &std::get<T>(*use_item_arena.create(std::move(value)));
    }

    SymbolTable *create_symbol_table(SymbolTable value) { return symbol_table_arena.create(std::move(value)); }
    OverloadSet *create_overload_set(OverloadSet value) { return overload_set_arena.create(std::move(value)); }
    Attributes *create_attrs(Attributes value) { return attributes_arena.create(std::move(value)); }
};

struct Unit {
    utils::GrowableArena<Module> mod_arena;
    std::vector<Module *> mods;
    std::unordered_map<ModulePath, Module *> mods_by_path;

    Module *create_mod() { return mod_arena.create(); }
};

template <typename T>
bool Expr::is_symbol() const {
    if (auto symbol_expr = match<SymbolExpr>()) {
        return symbol_expr->symbol.match<T>();
    } else {
        return false;
    }
}

template <typename T>
T &Expr::as_symbol() {
    return as<SymbolExpr>().symbol.as<T>();
}

template <typename T>
T *Expr::match_symbol() {
    if (auto symbol_expr = match<SymbolExpr>()) {
        return symbol_expr->symbol.match<T>();
    } else {
        return nullptr;
    }
}

template <typename T>
const T *Expr::match_symbol() const {
    if (auto symbol_expr = match<SymbolExpr>()) {
        return symbol_expr->symbol.match<T>();
    } else {
        return nullptr;
    }
}

} // namespace sir

} // namespace lang

} // namespace banjo

#endif
