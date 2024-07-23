#ifndef SIR_H
#define SIR_H

#include "banjo/symbol/module_path.hpp"
#include "banjo/utils/growable_arena.hpp"
#include "banjo/utils/large_int.hpp"

#include <cstddef>
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

struct IntLiteral;
struct FPLiteral;
struct BoolLiteral;
struct CharLiteral;
struct StringLiteral;
struct StructLiteral;
struct SymbolExpr;
struct BinaryExpr;
struct UnaryExpr;
struct CastExpr;
struct IndexExpr;
struct CallExpr;
struct FieldExpr;
struct RangeExpr;
struct FuncType;
struct PrimitiveType;
struct PointerType;
struct IdentExpr;
struct StarExpr;
struct BracketExpr;
struct DotExpr;
struct VarStmt;
struct AssignStmt;
struct ReturnStmt;
struct IfStmt;
struct WhileStmt;
struct ForStmt;
struct LoopStmt;
struct ContinueStmt;
struct BreakStmt;
struct FuncDef;
struct NativeFuncDecl;
struct StructDef;
struct StructField;
struct VarDecl;
struct Param;
struct UseDecl;
struct UseIdent;
struct UseDotExpr;
struct UseRebind;
struct UseList;

struct Block;
struct SymbolTable;
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
        StringLiteral *,
        StructLiteral *,
        SymbolExpr *,
        BinaryExpr *,
        UnaryExpr *,
        CastExpr *,
        IndexExpr *,
        CallExpr *,
        FieldExpr *,
        RangeExpr *,
        PrimitiveType *,
        PointerType *,
        FuncType *,
        IdentExpr *,
        StarExpr *,
        BracketExpr *,
        DotExpr *,
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

    template <typename T>
    T *match_symbol();

    operator bool() const { return !std::holds_alternative<std::nullptr_t>(kind); }

    bool is_value() const;
    Expr get_type() const;

    bool is_type() const;
    bool is_int_type() const;
    bool is_signed_type() const;
    bool is_unsigned_type() const;
    bool is_fp_type() const;
};

class Stmt {

private:
    std::variant<
        VarStmt *,
        AssignStmt *,
        ReturnStmt *,
        IfStmt *,
        WhileStmt *,
        ForStmt *,
        LoopStmt *,
        ContinueStmt *,
        BreakStmt *,
        Expr *,
        Block *>
        kind;

public:
    template <typename T>
    Stmt(T kind) : kind(kind) {}

    template <typename T>
    bool is() const {
        return std::holds_alternative<T *>(kind);
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
};

class Decl {

private:
    std::variant<FuncDef *, NativeFuncDecl *, StructDef *, StructField *, VarDecl *, UseDecl *, std::nullptr_t> kind;

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
};

class Symbol {
    std::variant<
        Module *,
        FuncDef *,
        NativeFuncDecl *,
        StructDef *,
        StructField *,
        VarDecl *,
        UseIdent *,
        UseRebind *,
        VarStmt *,
        Param *,
        std::nullptr_t>
        kind;

public:
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

    const std::string &get_name() const;
    Expr get_type();
    Symbol resolve();
    SymbolTable *get_symbol_table();
};

class UseItem {
    std::variant<UseIdent *, UseRebind *, UseDotExpr *, UseList *> kind;

public:
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

    Symbol look_up(std::string_view name);
};

struct Ident {
    ASTNode *ast_node;
    std::string value;
};

struct Param {
    ASTNode *node;
    Ident name;
    Expr type;
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

struct SymbolExpr {
    ASTNode *ast_node;
    Expr type;
    Symbol symbol;
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
    StructField *field;
};

struct RangeExpr {
    ASTNode *ast_node;
    Expr lhs;
    Expr rhs;
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
    F32,
    F64,
    BOOL,
    ADDR,
    VOID,
};

struct PrimitiveType {
    ASTNode *ast_node;
    Primitive primitive;
};

struct PointerType {
    ASTNode *ast_node;
    Expr base_type;
};

struct FuncType {
    ASTNode *ast_node;
    std::vector<Param> params;
    Expr return_type;
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

struct FuncDef {
    ASTNode *ast_node;
    Ident ident;
    FuncType type;
    Block block;

    bool is_main() const;
};

struct NativeFuncDecl {
    ASTNode *ast_node;
    Ident ident;
    FuncType type;
};

struct StructDef {
    ASTNode *ast_node;
    Ident ident;
    DeclBlock block;
    std::vector<StructField *> fields;

    StructField *find_field(std::string_view name) const;
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

struct UseDecl {
    ASTNode *ast_node;
    UseItem root_item;
};

struct UseIdent {
    ASTNode *ast_node;
    std::string value;
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
    StringLiteral,
    StructLiteral,
    SymbolExpr,
    BinaryExpr,
    UnaryExpr,
    CastExpr,
    IndexExpr,
    CallExpr,
    FieldExpr,
    RangeExpr,
    PrimitiveType,
    PointerType,
    FuncType,
    IdentExpr,
    StarExpr,
    BracketExpr,
    DotExpr>
    ExprStorage;

typedef std::
    variant<VarStmt, AssignStmt, ReturnStmt, IfStmt, WhileStmt, ForStmt, LoopStmt, ContinueStmt, BreakStmt, Expr, Block>
        StmtStorage;

typedef std::variant<FuncDef, NativeFuncDecl, StructDef, StructField, VarDecl, UseDecl> DeclStorage;

typedef std::variant<UseIdent, UseRebind, UseDotExpr, UseList> UseItemStorage;

struct Module {
    utils::GrowableArena<ExprStorage> expr_arena;
    utils::GrowableArena<StmtStorage> stmt_arena;
    utils::GrowableArena<DeclStorage> decl_arena;
    utils::GrowableArena<UseItemStorage> use_item_arena;
    utils::GrowableArena<SymbolTable> symbol_table_arena;

    ModulePath path;
    DeclBlock block;
    std::vector<sir::Module *> sub_mods;

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
};

struct Unit {
    utils::GrowableArena<Module> mod_arena;
    std::vector<Module *> mods;
    std::unordered_map<ModulePath, Module *> mods_by_path;

    Module *create_mod() { return mod_arena.create(); }
};

template <typename T>
T *Expr::match_symbol() {
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
