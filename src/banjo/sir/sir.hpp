#ifndef SIR_H
#define SIR_H

#include "banjo/source/module_path.hpp"
#include "banjo/utils/dynamic_pointer.hpp"
#include "banjo/utils/growable_arena.hpp"
#include "banjo/utils/large_int.hpp"

#include <cstddef>
#include <cstdint>
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

struct ASTNode;

namespace sir {

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
struct UnionCaseLiteral;
struct MapLiteral;
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
struct CoercionExpr;
enum class Primitive;
struct PrimitiveType;
struct PointerType;
struct StaticArrayType;
struct FuncType;
struct OptionalType;
struct ResultType;
struct ArrayType;
struct MapType;
struct ClosureType;
struct IdentExpr;
struct StarExpr;
struct BracketExpr;
struct DotExpr;
enum class PseudoTypeKind;
struct PseudoType;
struct MetaAccess;
struct MetaFieldExpr;
struct MetaCallExpr;
struct InitExpr;
struct MoveExpr;
struct DeinitExpr;
struct VarStmt;
struct AssignStmt;
struct CompAssignStmt;
struct ReturnStmt;
struct IfStmt;
struct SwitchStmt;
struct TryStmt;
struct WhileStmt;
struct ForStmt;
struct LoopStmt;
struct ContinueStmt;
struct BreakStmt;
struct MetaIfStmt;
struct MetaForStmt;
struct ExpandedMetaStmt;
struct FuncDef;
struct FuncDecl;
struct NativeFuncDecl;
struct ConstDef;
struct StructDef;
struct StructField;
struct VarDecl;
struct NativeVarDecl;
struct EnumDef;
struct EnumVariant;
struct UnionDef;
struct UnionCase;
struct ProtoDef;
struct TypeAlias;
struct UseDecl;
struct UseIdent;
struct UseDotExpr;
struct UseRebind;
struct UseList;
struct Error;

struct Unit;
struct DeclBlock;
struct Block;
struct SymbolTable;
struct OverloadSet;
struct Ident;
struct Module;
struct Param;
struct Local;
struct ProtoFuncDecl;

class Expr;
class Stmt;
class Decl;
class Symbol;
class UseItem;

constexpr std::string_view COMPLETION_TOKEN_VALUE = "[completion]";

enum class ExprCategory : std::uint8_t {
    VALUE,
    TYPE,
    VALUE_OR_TYPE,
    MODULE,
};

class Expr {
    std::variant<
        IntLiteral *,       // 0
        FPLiteral *,        // 1
        BoolLiteral *,      // 2
        CharLiteral *,      // 3
        NullLiteral *,      // 4
        NoneLiteral *,      // 5
        UndefinedLiteral *, // 6
        ArrayLiteral *,     // 7
        StringLiteral *,    // 8
        StructLiteral *,    // 9
        UnionCaseLiteral *, // 10
        MapLiteral *,       // 11
        ClosureLiteral *,   // 12
        SymbolExpr *,       // 13
        BinaryExpr *,       // 14
        UnaryExpr *,        // 15
        CastExpr *,         // 16
        IndexExpr *,        // 17
        CallExpr *,         // 18
        FieldExpr *,        // 19
        RangeExpr *,        // 20
        TupleExpr *,        // 21
        CoercionExpr *,     // 22
        PrimitiveType *,    // 23
        PointerType *,      // 24
        StaticArrayType *,  // 25
        FuncType *,         // 26
        OptionalType *,     // 27
        ResultType *,       // 28
        ArrayType *,        // 29
        MapType *,          // 30
        ClosureType *,      // 31
        IdentExpr *,        // 32
        StarExpr *,         // 33
        BracketExpr *,      // 34
        DotExpr *,          // 35
        PseudoType *,       // 36
        MetaAccess *,       // 37
        MetaFieldExpr *,    // 38
        MetaCallExpr *,     // 39
        InitExpr *,         // 40
        MoveExpr *,         // 41
        DeinitExpr *,       // 42
        Error *,            // 43
        std::nullptr_t>     // 44
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
    const T &as_symbol() const;

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

    ExprCategory get_category() const;
    bool is_type() const;
    bool is_primitive_type(Primitive primitive) const;
    bool is_int_type() const;
    bool is_signed_type() const;
    bool is_unsigned_type() const;
    bool is_fp_type() const;
    bool is_addr_like_type() const;
    const ProtoDef *match_proto_ptr() const;
    ProtoDef *match_proto_ptr();
    bool is_u8_ptr() const;
    bool is_symbol(sir::Symbol symbol) const;
    bool is_pseudo_type(PseudoTypeKind kind) const;

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
        SwitchStmt *,
        TryStmt *,
        WhileStmt *,
        ForStmt *,
        LoopStmt *,
        ContinueStmt *,
        BreakStmt *,
        MetaIfStmt *,
        MetaForStmt *,
        ExpandedMetaStmt *,
        Expr *,
        Block *,
        Error *,
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

    ASTNode *get_ast_node() const;
};

class Decl {

private:
    std::variant<
        FuncDef *,
        FuncDecl *,
        NativeFuncDecl *,
        ConstDef *,
        StructDef *,
        StructField *,
        VarDecl *,
        NativeVarDecl *,
        EnumDef *,
        EnumVariant *,
        UnionDef *,
        UnionCase *,
        ProtoDef *,
        TypeAlias *,
        UseDecl *,
        MetaIfStmt *,
        ExpandedMetaStmt *,
        Error *,
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

class Symbol : public DynamicPointer<
                   Module,
                   FuncDef,
                   FuncDecl,
                   NativeFuncDecl,
                   ConstDef,
                   StructDef,
                   StructField,
                   VarDecl,
                   NativeVarDecl,
                   EnumDef,
                   EnumVariant,
                   UnionDef,
                   UnionCase,
                   ProtoDef,
                   TypeAlias,
                   UseIdent,
                   UseRebind,
                   Local,
                   Param,
                   OverloadSet> {
public:
    Symbol() : DynamicPointer() {}

    template <typename T>
    Symbol(T value) : DynamicPointer(value) {}

    const Ident &get_ident() const;
    Ident &get_ident();
    std::string get_name() const;
    Symbol get_parent() const;
    Expr get_type();
    Symbol resolve() const;
    const SymbolTable *get_symbol_table() const;
    SymbolTable *get_symbol_table();
    const DeclBlock *get_decl_block() const;
    DeclBlock *get_decl_block();
};

class UseItem : public DynamicPointer<UseIdent, UseRebind, UseDotExpr, UseList, Error> {

public:
    UseItem() : DynamicPointer() {}

    template <typename T>
    UseItem(T value) : DynamicPointer(value) {}
};

class Node {

private:
    std::variant<Expr, Stmt, Decl, UseItem> kind;

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

enum class Ownership {
    OWNED,
    MOVED,
    MOVED_COND,
    INIT_COND,
};

struct Resource {
    sir::Expr type;
    bool has_deinit;
    Ownership ownership;
    std::vector<Resource> sub_resources;

    unsigned field_index;
};

struct Block {
    ASTNode *ast_node;
    std::vector<Stmt> stmts;
    SymbolTable *symbol_table;
    std::vector<std::pair<Symbol, Resource>> resources;
};

struct SymbolTable {
    SymbolTable *parent;
    std::unordered_map<std::string_view, Symbol> symbols;
    std::vector<Symbol> local_symbols_ordered;

    void insert_decl(std::string_view name, Symbol symbol);
    void insert_local(std::string_view name, Symbol symbol);

    Symbol look_up(std::string_view name) const;
    Symbol look_up_local(std::string_view name) const;
};

struct OverloadSet {
    std::vector<FuncDef *> func_defs;
};

struct Attributes {
    enum class Layout : std::uint8_t {
        DEFAULT,
        PACKED,
        OVERLAPPING,
        C,
    };

    bool exposed = false;
    bool dllexport = false;
    bool test = false;
    std::optional<std::string> link_name = {};
    bool unmanaged = false;
    bool byval = false;
    std::optional<Layout> layout = {};
};

struct Ident {
    ASTNode *ast_node;
    std::string value;

    bool is_completion_token() const { return value == COMPLETION_TOKEN_VALUE; }
};

struct Param {
    ASTNode *ast_node;
    Ident name;
    Expr type;
    Attributes *attrs = nullptr;

    bool is_self() const { return name.value == "self"; }

    bool operator==(const Param &other) const { return type == other.type; }
    bool operator!=(const Param &other) const { return !(*this == other); }
};

enum class GenericParamKind {
    TYPE,
    SEQUENCE,
};

struct GenericParam {
    ASTNode *ast_node;
    Ident ident;
    GenericParamKind kind;
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

    bool is_func_method() const { return params.size() > 0 && params[0].is_self(); }

    bool operator==(const FuncType &other) const { return params == other.params && return_type == other.return_type; }
    bool operator!=(const FuncType &other) const { return !(*this == other); }
};

struct Local {
    Ident name;
    Expr type;
    Attributes *attrs = nullptr;
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

struct UnionCaseLiteral {
    ASTNode *ast_node;
    Expr type;
    std::vector<Expr> args;
};

struct MapLiteralEntry {
    Expr key;
    Expr value;
};

struct MapLiteral {
    ASTNode *ast_node;
    Expr type;
    std::vector<MapLiteralEntry> entries;
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

struct CoercionExpr {
    ASTNode *ast_node;
    Expr type;
    Expr value;
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
    USIZE,
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

struct ResultType {
    ASTNode *ast_node;
    Expr value_type;
    Expr error_type;

    bool operator==(const ResultType &other) const {
        return value_type == other.value_type && error_type == other.error_type;
    }

    bool operator!=(const ResultType &other) const { return !(*this == other); }
};

struct ArrayType {
    ASTNode *ast_node;
    Expr base_type;

    bool operator==(const ArrayType &other) const { return base_type == other.base_type; }
    bool operator!=(const ArrayType &other) const { return !(*this == other); }
};

struct MapType {
    ASTNode *ast_node;
    Expr key_type;
    Expr value_type;

    bool operator==(const MapType &other) const { return key_type == other.key_type && value_type == other.value_type; }
    bool operator!=(const MapType &other) const { return !(*this == other); }
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

    bool is_completion_token() const { return value == COMPLETION_TOKEN_VALUE; }
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
    MAP_LITERAL,
};

struct PseudoType {
    ASTNode *ast_node;
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

struct InitExpr {
    ASTNode *ast_node;
    Expr type;
    Expr value;
    Resource *resource;
};

struct MoveExpr {
    ASTNode *ast_node;
    Expr type;
    Expr value;
    Resource *resource;
};

struct DeinitExpr {
    ASTNode *ast_node;
    Expr type;
    Expr value;
    Resource *resource;
};

struct ProtoCallExpr {
    ASTNode *ast_node;
    Expr type;
    Expr value;
};

struct VarStmt {
    ASTNode *ast_node;
    Local local;
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

struct SwitchCaseBranch {
    ASTNode *ast_node;
    Local local;
    Block block;
};

struct SwitchStmt {
    ASTNode *ast_node;
    Expr value;
    std::vector<SwitchCaseBranch> case_branches;
};

struct TrySuccessBranch {
    ASTNode *ast_node;
    Ident ident;
    Expr expr;
    Block block;
};

struct TryExceptBranch {
    ASTNode *ast_node;
    Ident ident;
    Expr type;
    Block block;
};

struct TryElseBranch {
    ASTNode *ast_node;
    Block block;
};

struct TryStmt {
    ASTNode *ast_node;
    TrySuccessBranch success_branch;
    std::optional<TryExceptBranch> except_branch;
    std::optional<TryElseBranch> else_branch;
};

struct WhileStmt {
    ASTNode *ast_node;
    Expr condition;
    Block block;
};

struct ForStmt {
    ASTNode *ast_node;
    bool by_ref;
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

struct MetaForStmt {
    ASTNode *ast_node;
    Ident ident;
    Expr range;
    MetaBlock block;
};

struct ExpandedMetaStmt {};

struct FuncDef {
    ASTNode *ast_node;
    Ident ident;
    Symbol parent;
    FuncType type;
    Block block;
    Attributes *attrs = nullptr;
    std::vector<GenericParam> generic_params;
    std::list<Specialization<FuncDef>> specializations;
    Specialization<FuncDef> *parent_specialization;

    Module &find_mod() const;
    bool is_generic() const { return !generic_params.empty(); }
    bool is_main() const { return ident.value == "main"; }
    bool is_method() const { return type.is_func_method(); }
};

struct FuncDecl {
    ASTNode *ast_node;
    Ident ident;
    Symbol parent;
    FuncType type;

    bool is_method() const { return type.is_func_method(); }
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
    Symbol parent;
    DeclBlock block;
    std::vector<StructField *> fields;
    std::vector<Expr> impls;
    Attributes *attrs = nullptr;
    std::vector<GenericParam> generic_params;
    std::list<Specialization<StructDef>> specializations;
    Specialization<StructDef> *parent_specialization;

    Module &find_mod() const;
    StructField *find_field(std::string_view name) const;
    bool has_impl_for(const sir::ProtoDef &proto_def) const;
    Attributes::Layout get_layout() const;
    bool is_generic() const { return !generic_params.empty(); }
};

struct StructField {
    ASTNode *ast_node;
    Ident ident;
    Expr type;
    Attributes *attrs = nullptr;
    unsigned index;
};

struct VarDecl {
    ASTNode *ast_node;
    Ident ident;
    Expr type;
    Expr value;
    Attributes *attrs = nullptr;
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

struct UnionDef {
    ASTNode *ast_node;
    Ident ident;
    DeclBlock block;
    std::vector<UnionCase *> cases;

    unsigned get_index(const UnionCase &case_) const;
};

struct UnionCaseField {
    ASTNode *ast_node;
    Ident ident;
    Expr type;
};

struct UnionCase {
    ASTNode *ast_node;
    Ident ident;
    std::vector<UnionCaseField> fields;

    std::optional<unsigned> find_field(std::string_view name) const;
};

struct ProtoDef {
    ASTNode *ast_node;
    Ident ident;
    DeclBlock block;
    std::vector<ProtoFuncDecl> func_decls;

    std::optional<unsigned> get_index(std::string_view name) const;
};

struct ProtoFuncDecl {
    Decl decl;

    ASTNode *get_ast_node() const;
    const Ident &get_ident() const;
    Symbol get_parent() const;
    FuncType &get_type();
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

    bool is_completion_token() const { return ident.is_completion_token(); }
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

struct Error {
    ASTNode *ast_node;
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
    UnionCaseLiteral,
    MapLiteral,
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
    CoercionExpr,
    PrimitiveType,
    PointerType,
    StaticArrayType,
    FuncType,
    OptionalType,
    ResultType,
    ArrayType,
    MapType,
    ClosureType,
    IdentExpr,
    StarExpr,
    BracketExpr,
    DotExpr,
    PseudoType,
    MetaAccess,
    MetaFieldExpr,
    MetaCallExpr,
    InitExpr,
    MoveExpr,
    DeinitExpr,
    Error>
    ExprStorage;

typedef std::variant<
    VarStmt,
    AssignStmt,
    CompAssignStmt,
    ReturnStmt,
    IfStmt,
    SwitchStmt,
    TryStmt,
    WhileStmt,
    ForStmt,
    LoopStmt,
    ContinueStmt,
    BreakStmt,
    MetaIfStmt,
    MetaForStmt,
    ExpandedMetaStmt,
    Expr,
    Block,
    Error>
    StmtStorage;

typedef std::variant<
    FuncDef,
    FuncDecl,
    NativeFuncDecl,
    ConstDef,
    StructDef,
    StructField,
    VarDecl,
    NativeVarDecl,
    EnumDef,
    EnumVariant,
    UnionDef,
    UnionCase,
    ProtoDef,
    TypeAlias,
    UseDecl,
    Error>
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
    utils::GrowableArena<Resource> resource_arena;

    ModulePath path;
    DeclBlock block;

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
    Resource *create_resource(Resource value) { return resource_arena.create(std::move(value)); }
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
const T &Expr::as_symbol() const {
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

template <>
struct std::hash<banjo::lang::sir::Symbol> {
    std::size_t operator()(const banjo::lang::sir::Symbol &symbol) const noexcept { return symbol.compute_hash(); }
};

#endif
